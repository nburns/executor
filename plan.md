# Executor arm64/SDL2 port — remaining work

## Status

| Phase | What | State |
|---|---|---|
| 1 | Header GET_/SET_ aliases for undeclared low globals | ✓ Done |
| 2 | Direct `.p` member access on HIDDEN_* types | ✓ Done |
| 3 | Header macro aliases + union arithmetic | **Next** |
| 4 | SDL_bmp.c SDL2 API fixes | ✓ Done |
| 5 | Misc: alias.c macro, dump.c casts | After Phase 3 |
| 7 | Runtime testing with skel volume | After Phase 5 |
| 6 | arm64 JIT for syn68k | Optional / large project |

Build is configured in `build-sdl/` with `FORCE_EXPERIMENTAL_PACKED_MACROS 1` and SDL2 front-end. Run `./errors.sh` to see current errors grouped by category.

Current error count (post-Phase-2): ~74 total. Most are Phase 3 header macro errors now visible after Phase 2 fixes unmasked them.

---

## Root cause

Executor stores Mac 32-bit addresses in `HIDDEN_*` union types. On 32-bit hosts the union has a `.p` native pointer field. On 64-bit (arm64) it has only `.pp` (a `uint32_t` Mac address) — the `.p` field does not exist and every direct access fails to compile.

The fix layer (`STARH`, `HPTR_WRITE`, `GET_`/`SET_` macros, etc.) is defined in `byteswap.h`. Phases 1–3 are about finishing the application of that layer across the codebase.

---

## Phase 2 — Direct `.p` member access ✓ Done

All direct `h->p` / `val.p` accesses on HIDDEN_* types in the 23 source files have been converted to the portable accessor macros. See macro cheat sheet below.

Files fixed: `executor.c`, `osevent.c`, `mmansubr.c`, `syserr.c`, `crc.c`, `iu.c`, `appearance.c`, `emustubs.c`, `toolutil.c`, `icon.c`, `system_error.c`, `aboutbox.c`, `toolevent.c`, `stdfile.c`, `segment.c`, `font.c`, `osutil.c`, `device.c`, `desk.c`, `launch.c`, `main.c`.

Note: `scrap.c` and `mman.c` had no direct `.p` in source — all their errors came from header macros (Phase 3).

---

## Phase 3 — Header macro aliases + union arithmetic

Two sub-problems, both must be fixed to clear the remaining ~74 errors:

### 3a — Header macros with 32-bit-only `.p` aliases

Headers still needing 64-bit GET_/SET_ branches:

| Header | Globals |
|---|---|
| `DeviceMgr.h` | `UTableBase`, `VIA` |
| `QuickDraw.h` | `JInitCrsr`, `JHideCursor`, `JShowCursor`, `JShieldCursor`, `JSetCrsr`, `JCrsrObscure`, `JUnknown574`, `JCrsrTask`, `ScrnBase`, `Key1Trans`, `Key2Trans` |
| `rsys/jumpvectors.h` | `JFLUSH`, `JResUnknown1`, `JResUnknown2` |
| `MenuMgr.h` | `MenuList`, `MBarHook`, `MenuHook`, `MBDFHndl`, `MBSaveLoc`, `MenuCInfo` |
| `SysErr.h` | `DSAlertTab` |
| `rsys/misc.h` | `nilhandle`, `dodusesit` |
| `TextEdit.h` | `TEDoText`, `TEScrpHandle` |
| `rsys/mman_private.h` | `HANDLE_TO_BLOCK` macro uses `(handle)->p` |
| `ScrapMgr.h` | `ScrapHandle`, `ScrapName` |
| `SoundMgr.h` / low globals | `SoundBase` |
| `AppleEvents.h` | `AE_info` |
| `FontMgr.h` | `fontHandle` field in `FMOutput` |

Pattern (same as Phase 1):
```c
# define FooGlobal        GET_FooGlobal()
# define GET_FooGlobal()  ((FooType) PPR(FooGlobal_H))
# define SET_FooGlobal(v) (FooGlobal_H.pp = RPP(v))
```

`UTableBase` and `VIA` also need SET_ variants since they appear as lvalues in `device.c` and `launch.c`.

### 3b — Union arithmetic errors

`HIDDEN_*` values used in comparisons or arithmetic fail with:
```
error: invalid operands to binary expression ('int' and 'union ...')
```

Files: `dump.c`, `gestalt.c`, `font.c` (fontHandle field), `device.c` (ioNamePtr in FileMgr.h).

Fix: use `HIDDEN_VAL(x)` for integer comparisons, `PPR(x)` to get the native pointer, or `HxZ`/`HxP` for handle field access.

---

## Phase 5 — Misc

- `alias.c:469` — macro expands to undeclared `n`; check `N()` macro definition and include guards
- `dump.c` — union/pointer type errors from HIDDEN_* fields used in printf-style accessors; fix case by case
- `crc.c` — was listed here but Phase 2 fixes resolved it

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
