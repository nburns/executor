# Executor arm64 Port — Plan & Progress

## Goal

Port the Executor Mac 68k emulator to compile on arm64 macOS (Apple Silicon). Fix all
64-bit compilation errors caused by the HIDDEN/PACKED_MEMBER pointer-type system,
file by file, until reaching SDL-related errors.

---

## Key Technical Concepts

### PACKED_MEMBER / HIDDEN type system

On **32-bit**: `PACKED_MEMBER(T, name)` is just `T name` (plain field). HIDDEN_T is
`struct { T p; }` — access via `.p`.

On **64-bit**: `PACKED_MEMBER(T, name)` is `union PACKED { uint32 pp; T type[0]; } name`.
HIDDEN_T is the same union. No `.p` field exists.

### Key macros (64-bit relevant)

| Macro | Purpose |
|-------|---------|
| `PPR(x)` | Extract native pointer from PACKED union |
| `RPP(p)` | Convert native pointer → uint32 Mac address |
| `FROM_HIDDEN(h)` | HIDDEN value → native pointer (both archs) |
| `HPTR_WRITE(hptr, v)` | Write native ptr into HIDDEN_Ptr |
| `PACKED_ASSIGN(lval, ptr)` | Store native ptr into PACKED_MEMBER |
| `PACKED_ASSIGN0(lval)` | Zero a PACKED_MEMBER |
| `GET_WindowList()` | Read WindowList (64-bit safe) |
| `GET_GrayRgn()` | Read GrayRgn (64-bit safe) |
| `GET_TheZone()` / `SET_TheZone(v)` | TheZone access (both archs) |
| `GET_SysZone()` | SysZone read (both archs) |
| `active_screen_addr_p(bm)` | Compare baseAddr against screen (fixed in cquick.h) |

### Save/restore pattern for PACKED fields (64-bit)

```c
uint32_t saved = PORT_PIC_SAVE_X(thePort).pp;
PACKED_ASSIGN0(PORT_PIC_SAVE_X(thePort));
// ... use ...
PORT_PIC_SAVE_X(thePort).pp = saved;
```

### Common fix patterns

- `MR(x->packedField)` → `PPR(x->packedField)`
- `x->packedField = RM(ptr)` → `PACKED_ASSIGN(x->packedField, ptr)`
- `x->packedField = 0` or `= CLC(0)` → `PACKED_ASSIGN0(x->packedField)`
- `x.p` (HIDDEN) → `x.pp` (on 64-bit)
- `MR(WindowList)` → `GET_WindowList()`
- `TheZone = SysZone` → `SET_TheZone(GET_SysZone())`
- `auto T name;` (C23 breaks `auto` storage class) → `T name;`

---

## Build command

```
cd /Users/nick/code/executor/build && make -j1
```

---

## Files Fixed (compiling)

| File | Key changes |
|------|------------|
| `src/include/rsys/next.h` | `SETUPA5` macro uses `CurrentA5_H.pp` on 64-bit |
| `src/PSprint.c` | K&R fn fix; `PPR(srcbmp->baseAddr)`; `PACKED_ASSIGN(pxp,...)` |
| `src/prLowLevel.c` | `FROM_HIDDEN(h)` for HIDDEN_Handle; PPR for PACKED fields |
| `src/prPrinting.c` | ~28 QDProcs field assignments → `PACKED_ASSIGN` |
| `src/include/rsys/picture.h` | `PICSAVEBEGIN`, `PAUSERECORDING` etc. conditional macros |
| `src/include/rsys/cquick.h` | `PIXPAT_*_X` via `HxX`; `AS_OFFSET` uses `.pp`; `active_screen_addr_p` uses `.pp` comparison |
| `src/qCGrafPort.c` | PACKED_ASSIGN throughout; `.pp == 0` null checks |
| `src/qCRegular.c` | `FillCxxx` macro save/restore with `uint32_t` and `.pp` |
| `src/qColorMgr.c` | Save/restore with `uint32_t`; SProcHndl list ops |
| `src/qCursor.c` | CCrsr fields; `STARH` for handle content |
| `src/include/WindowMgr.h` | Added `SET_WMgrPort`, `SET_WMgrCPort` |
| `src/include/QuickDraw.h` | Added `SET_ScrnBase` |
| `src/qGrafport.c` | WMgrPort/CPort/SaveVisRgn via GET_/SET_; `RPP(p)` in SetPort |
| `src/qIMIV.c` | `PPR(screenBitsX.baseAddr)`; uint32_t save/restore for pic/graf procs |
| `src/qIMV.c` | `PACKED_ASSIGN` for CQDProcs fields; uint32_t save/restore; `PPR` for baseAddr |
| `src/qIMVxfer.c` | `PPR(->baseAddr)` in pointer arithmetic; `PAT_NEXT1` macro |
| `src/qMisc.c` | `PACKED_ASSIGN(temp_bm.baseAddr, ...)` |
| `src/qPaletteMgr.c` | `GET_WindowList/GrayRgn`; `.pp` comparisons; `PACKED_ASSIGN` |
| `src/qPicstuff.c` | Many fixes: `auto` removal, `TheZone→GET_TheZone`, HIDDEN_Ptr loops with `#if`, PACKED_ASSIGN throughout |
| `src/qPicture.c` | `PACKED_ASSIGN` for picSave/pichandle/picclip; `PORT_PIC_SAVE()` |
| `src/qPixMapConv.c` | `GD_PMAP(gdh)` instead of `MR(gd->gdPMap)`; `PPR` for baseAddr/pmTable |
| `src/qPoly.c` | `PACKED_ASSIGN` for PORT_POLY_SAVE_X |

---

## Currently Failing

**`src/qRegion.c`** — two classes of errors:

1. Lines 103, 215: `PORT_REGION_SAVE_X(thePort) = (Handle) RM(rh)` and `= RM(NULL)`
   - Fix: `PACKED_ASSIGN(...)` / `PACKED_ASSIGN0(...)`

2. Lines 1219–1258: `temp2.p` and `temp3.p` accessed on `HIDDEN_RgnPtr` locals
   - Pattern: `temp2.p = (RgnPtr) ALLOCA(...)` then `temp2.p = RM(temp2.p)` then `s1 = &temp2`
   - These are HIDDEN_RgnPtr locals used as pass-by-ref args to region combine routines
   - Fix: use `#if` conditional (same as eatbitdata in qPicstuff.c), or use `.pp` directly

---

## Remaining Work (after qRegion.c)

After qRegion.c compiles, the build will proceed to more QuickDraw files. Expected
pattern: continue applying the same PACKED_ASSIGN / PPR / `.pp` fixes file by file
through:

- qStdBits.c, qStdRgn.c, qStdText.c — more baseAddr / pmTable / grafProcs issues
- srcblt.c, xdblt.c, xdata.c — baseAddr arithmetic
- vdriver/ and SDL front-end files — may hit SDL1→SDL2 migration issues (the "SDL" stop point)

When SDL errors appear (undefined SDL1 functions, wrong API signatures), stop and
report — those require a separate migration effort.
