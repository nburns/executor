# Executor arm64/SDL2 port — remaining work

## Status

| Phase | What | State |
|---|---|---|
| 1 | Header GET_/SET_ aliases for undeclared low globals | ✓ Done |
| 2 | Direct `.p` member access on HIDDEN_* types | **Next** |
| 3 | Union arithmetic on HIDDEN_* values | After Phase 2 |
| 4 | SDL_bmp.c SDL2 API fixes | ✓ Done |
| 5 | Misc: alias.c macro, dump.c casts, crc.c | After Phase 3 |
| 7 | Runtime testing with skel volume | After Phase 5 |
| 6 | arm64 JIT for syn68k | Optional / large project |

Build is configured in `build-sdl/` with `FORCE_EXPERIMENTAL_PACKED_MACROS 1` and SDL2 front-end. Run `./errors.sh` to see current errors grouped by category.

---

## Root cause

Executor stores Mac 32-bit addresses in `HIDDEN_*` union types. On 32-bit hosts the union has a `.p` native pointer field. On 64-bit (arm64) it has only `.pp` (a `uint32_t` Mac address) — the `.p` field does not exist and every direct access fails to compile.

The fix layer (`STARH`, `HPTR_WRITE`, `GET_`/`SET_` macros, etc.) is defined in `byteswap.h`. Phases 1–3 are about finishing the application of that layer across the codebase.

---

## Phase 2 — Direct `.p` member access

Errors come from two sources:

1. **Header macros** with 32-bit-only `.p` aliases (no 64-bit `#else` branch). Headers still needing 64-bit branches:

| Header | Globals |
|---|---|
| `DeviceMgr.h` | `UTableBase`, `VIA`, `UnitNtryCnt` |
| `QuickDraw.h` | `JInitCrsr`, `JHideCursor`, `JShowCursor`, `JShieldCursor`, `JSetCrsr`, `JCrsrObscure`, `JUnknown574`, `JCrsrTask`, `ScrnBase`, `Key1Trans`, `Key2Trans` |
| `rsys/jumpvectors.h` | `JFLUSH`, `JResUnknown1`, `JResUnknown2` |
| `MenuMgr.h` | `MenuList`, `MBarHook`, `MenuHook`, `MBDFHndl`, `MBSaveLoc`, `MenuCInfo` |
| `SysErr.h` | `DSAlertTab` |
| `rsys/misc.h` | `nilhandle` |
| `TextEdit.h` | `TEDoText`, `TEScrpHandle` |

Pattern (same as Phase 1):
```c
# define FooGlobal       GET_FooGlobal()
# define GET_FooGlobal() ((FooType) PPR(FooGlobal_H))
# define SET_FooGlobal(v) (FooGlobal_H.pp = RPP(v))
```

2. **Explicit `.p` in source** — 23 files with direct `h->p` / `val.p` access on HIDDEN_ types.

Substitution rules:

| Old | New | Notes |
|---|---|---|
| `MR(h->p)` or `h->p` read | `STARH(h)` | h is HIDDEN_T * |
| `h->p = RM(expr)` | `HPTR_WRITE(h, expr)` | drop the RM |
| `h->p = NULL/CLC(0)` | `HPTR_WRITE0(h)` | |
| `MR(val.p)` or `val.p` as pointer | `FROM_HIDDEN(val)` | val is HIDDEN_T value |
| `val.p` as Mac uint32 | `HIDDEN_VAL(val)` | |
| `val.p = RM(expr)` | `HIDDEN_VAL_WRITE(val, expr)` | drop the RM |
| `val.p = 0/NULL` | `HIDDEN_VAL_WRITE0(val)` | |
| `sizeof(someHidden.p)` | `sizeof(Ptr)` | one instance: `main.c:1939` |

**Failing files** (by error count):
```
launch.c (13)  executor.c (9)  main.c (8)    osutil.c (8)
syserr.c (7)   toolevent.c (7) toolutil.c (7) segment.c (7)
scrap.c (6)    device.c (5)    font.c (4)    stdfile.c (4)
desk.c (3)     emustubs.c (3)  icon.c (3)    mman.c (3)
mmansubr.c (3) osevent.c (3)   system_error.c (3) appearance.c (2)
aboutbox.c (1) crc.c (1)       iu.c (1)
```

**CAUTION:** `.p` also exists on non-HIDDEN types (`obj->u.cfdib.p` in SDL_bmp.c, `rp.p` in qStdLine.c, etc.). Scope rewrites to the 23 files above only.

---

## Phase 3 — Union arithmetic errors

`HIDDEN_*` values used in comparisons or arithmetic fail with:
```
error: invalid operands to binary expression ('int' and 'union ...')
```

Fix: use `HIDDEN_VAL(x)` for integer comparisons, `PPR(x)` to get the native pointer, or `HxZ`/`HxP` for handle field access.

---

## Phase 5 — Misc

- `alias.c:469` — macro expands to undeclared `n`; check `N()` macro definition and include guards
- `dump.c` — union/pointer type errors from HIDDEN_* fields used in printf-style accessors; fix case by case
- `crc.c` — similar

---

## Phase 7 — Runtime

```bash
cp -Rp /usr/local/share/executor/skel/volume /tmp/ExecutorVolume
export SystemFolder="/tmp/ExecutorVolume/System Folder"
./build-sdl/executor
```

Expected issues: test with `-nosound` first (SDL2 semaphores); verify clipboard round-trip; mouse tracking under macOS SDL2 event model.

---

## Macro cheat sheet

```c
// Handle (HIDDEN_T *h):
STARH(h)              // h->pp translated to native T*
HxP(h, field)         // read pointer field via handle
HxX(h, field)         // read non-pointer field via handle
HxZ(h, field)         // test field for zero
HPTR_WRITE(h, ptr)    // h->pp = RPP(ptr)
HPTR_WRITE0(h)        // h->pp = 0

// Value (HIDDEN_T val):
FROM_HIDDEN(val)           // PPR(val) - to native ptr
HIDDEN_VAL(val)            // val.pp   - raw Mac uint32
HIDDEN_VAL_WRITE(val, ptr) // val.pp = RPP(ptr)
HIDDEN_VAL_WRITE0(val)     // val.pp = 0
```
