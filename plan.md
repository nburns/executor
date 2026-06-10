# Executor arm64/SDL2 port — remaining work

## State as of now

The SDL2 front-end is complete and compiles cleanly. The build is configured
in `build-sdl/` with `--with-front-end=sdl`, syn68k using the `native/null`
interpreter backend (arm64 has no JIT), SDL2 2.32.10 via Homebrew.

**Phase 1 is complete.** ~474 errors remain, all in Phase 2+.

Error counts after Phase 1:

| Category | Count |
|---|---|
| `no member named 'p' in 'HIDDEN_*'` | ~230 (110 unique locations) |
| Union arithmetic / incompatible type | ~30 |
| `SDL_AllocSurface` (Phase 4) | 2 |
| `SDL_SetTimer`, `n` undeclared (Phase 5) | 2 |
| Pre-existing CQuickDraw lvalue (`TheGDevice = ...`) | 1 |

---

## Root cause

Executor uses `HIDDEN_*` types to store Mac 32-bit addresses in a way that
works on both 32-bit (host addr == Mac addr) and 64-bit (require translation).

On arm64 (`SIZEOF_CHAR_P == 8`):

```c
// MacTypes.h / byteswap.h
MAKE_HIDDEN(Ptr)  →  typedef union PACKED { uint32 pp; Ptr type[0]; } HIDDEN_Ptr
```

The `.p` field exists only in the 32-bit struct form. On 64-bit the field is
`.pp` (a Mac 32-bit address) and reads require `PPR()`/`STARH()` to translate
to a host pointer. All source files that use `.p` directly, or reference low
globals without their GET_/SET_ accessor macros, fail.

The accessor layer (GET_/SET_ macros, PPR, RPP, STARH, HPTR_WRITE, etc.) is
already defined in `byteswap.h` and partially applied in the headers. What
remains is finishing the job.

---

## Error inventory

| Category | Count | Fix |
|---|---|---|
| `no member named 'p' in 'HIDDEN_*'` | 195 | Replace `.p` reads/writes with STARH/HPTR_WRITE/etc. |
| Undeclared globals (`TheZone`, `ApplLimit`, etc.) | ~40 | Add GET_ aliases to headers |
| Union arithmetic errors | ~30 | Wrap HIDDEN fields in HIDDEN_VAL() before arithmetic |
| `SDL_AllocSurface`, `SDL_Color.unused` | 4 | SDL_bmp.c fixes (CYGWIN32-only code) |
| Misc (`n` undeclared, `alias.c`, `dump.c`) | ~10 | Case by case |

---

## Phase 1 — Header aliases for undeclared globals ✓ DONE

**Headers modified:** `MemoryMgr.h`, `WindowMgr.h`, `FileMgr.h`, `ResourceMgr.h`,
`DialogMgr.h`.

Added GET_/SET_ macros and convenience aliases for all ~20 undeclared globals
to the 64-bit `#else` branch of each header. Also added missing SET_ macros
(`SET_SysZone`, `SET_heapcheck`, `SET_IAZNotify`, `SET_ResumeProc`,
`SET_ResErrProc`).

**Source files with assignment-site fixes:** `font.c`, `mmansubr.c`, `syserr.c`,
`tempmem.c`, `executor.c`, `segment.c`, `launch.c`, `main.c` — all bare
`TheZone = x` / `CurStackBase = x` etc. converted to `SET_TheZone(x)` form.

---

## Phase 2 — Direct `.p` member access (~230 error instances, 110 unique locations)

**Key finding from investigation:** The `.p` errors come from TWO distinct sources:

1. **Header-level macros** (majority): globals like `JInitCrsr`, `VIA`, `Key1Trans`,
   `UTableBase`, `JFLUSH`, `DSAlertTab`, `nilhandle` etc. are defined in headers
   with a 32-bit-only `X_H.p` alias and NO 64-bit `#else` branch. The source files
   look fine but fail because the macro expansion produces a `.p` access.

2. **Source-level explicit `.p` access**: code directly writes `h->p` or `value.p`
   on a HIDDEN_ type (e.g. `MR(UTableBase)[i].p`, `(*h).p`, `SCCRd_H.p`).

**Strategy — two-pass:**

### Pass A: Add 64-bit aliases to headers with 32-bit-only `.p` macros

Headers needing a 64-bit `#else` branch or alias (confirmed from error trace):

| Header | Globals missing 64-bit alias |
|---|---|
| `DeviceMgr.h` | `UTableBase`, `VIA`, `UnitNtryCnt` (check) |
| `QuickDraw.h` | `JInitCrsr`, `JHideCursor`, `JShowCursor`, `JShieldCursor`, `JSetCrsr`, `JCrsrObscure`, `JUnknown574`, `JCrsrTask`, `ScrnBase`, `Key1Trans`, `Key2Trans` |
| `rsys/jumpvectors.h` | `JFLUSH`, `JResUnknown1`, `JResUnknown2` |
| `MenuMgr.h` | `MenuList`, `MBarHook`, `MenuHook`, `MBDFHndl`, `MBSaveLoc`, `MenuCInfo` |
| `SysErr.h` | `DSAlertTab` |
| `rsys/misc.h` | `nilhandle` |
| `TextEdit.h` | `TEDoText`, `TEScrpHandle` |

Pattern for each: add to the 64-bit `#else` branch (or create one):
```c
# define FooGlobal     GET_FooGlobal()
# define GET_FooGlobal() ((FooType) PPR(FooGlobal_H))
# define SET_FooGlobal(v) (FooGlobal_H.pp = RPP(v))
```

For globals assigned as lvalues in source, also convert assignment sites to
`SET_FooGlobal(v)` — same pattern as Phase 1.

### Pass B: Source-level `.p` accesses on HIDDEN_ struct fields

After Pass A, remaining `.p` errors will be explicit accesses in source.
Conversion rules:

| Old (32-bit) | New (64-bit) | Notes |
|---|---|---|
| `MR(h->p)` | `STARH(h)` | h is HIDDEN_T * |
| `h->p` read | `STARH(h)` | same |
| `h->p = RM(expr)` | `HPTR_WRITE(h, expr)` | drop the RM |
| `h->p = NULL/CLC(0)` | `HPTR_WRITE0(h)` | |
| `MR(val.p)` | `FROM_HIDDEN(val)` | val is HIDDEN_T value |
| `val.p` read | `FROM_HIDDEN(val)` | if used as pointer |
| `val.p` read | `HIDDEN_VAL(val)` | if used as Mac uint32 address |
| `val.p = RM(expr)` | `HIDDEN_VAL_WRITE(val, expr)` | drop the RM |
| `val.p = 0/CLC(0)/NULL` | `HIDDEN_VAL_WRITE0(val)` | |

**CAUTION:** `.p` also exists on non-HIDDEN types in the codebase (e.g.
`obj->u.cfdib.p` in SDL_bmp.c, `rp.p` in qStdLine.c, etc.). Do NOT apply
ast-grep globally — scope rewrites to the 23 failing files only, and verify
each substitution makes sense in context.

**Failing files** (23, by unique error location count):
```
toolutil.c (7)   toolevent.c (7)  segment.c (7)   launch.c (13)
font.c (4)       mmansubr.c (3)   scrap.c (6)     osutil.c (8)
device.c (5)     desk.c (3)       syserr.c (7)    mman.c (3)
main.c (8)       executor.c (9)   emustubs.c (3)  osevent.c (3)
stdfile.c (4)    system_error.c (3) icon.c (3)    aboutbox.c (1)
appearance.c (2) crc.c (1)        iu.c (1)
```

Special case — `main.c:1939`: `sizeof(UTableBase[0].p)` needs to become
`sizeof(Ptr)` (getting the size of the pointer field for allocation).

---

## Phase 3 — Union arithmetic errors (~30 occurrences)

These errors come from expressions like `handle_field == 0` or arithmetic on a
`HIDDEN_*` union value instead of its translated pointer. Pattern:

```
error: invalid operands to binary expression ('int' and 'union (unnamed union at ...)')
```

The union type can't be compared to integers directly. Fix: use `HIDDEN_VAL(x)`
to get the raw `uint32` for comparisons, or `PPR(x)` to get the pointer.

Example:
```c
// Before
if (someHandle->someField == 0)

// After (comparing Mac address)
if (HxZ(someHandle, someField) == 0)
// or for a read-then-compare
if (HxP(someHandle, someField) == NULL)
```

Context matters — check each site. Most are NULL checks on pointer fields.

---

## Phase 4 — SDL_bmp.c (30 minutes)

This file is CYGWIN32-only but unconditionally compiled. Two approaches:

**Option A (preferred):** Guard in `sdl.make`:
```makefile
ifneq (,$(findstring mingw,$(HOST)))
  SDL_SRC += SDL_bmp.c
endif
```

**Option B:** Fix the SDL2 incompatibilities:
- `SDL_AllocSurface(...)` → `SDL_CreateRGBSurface(...)`
- `SDL_Color.unused` → `SDL_Color.a`

Option A avoids touching the CYGWIN32 code path entirely.

---

## Phase 5 — Misc remaining failures (~1 hour)

### `alias.c:469` — `unknown type name 'n'`

This is a macro expansion failure. The `N()` or similar macro expands to `n`
which isn't in scope. Check the macro definition site and the surrounding
include guard conditions.

### `dump.c` errors

`dump.c` has C++ style casts or similar. Check and fix individually.

### `crc.c` errors

Similar — check and fix individually.

---

## Phase 6 — syn68k interpreter performance

The arm64 build uses syn68k's `native/null` interpreter backend — there is no
JIT for arm64. The 68k emulation runs roughly 100-1000x slower than it would
on a native x86 system with JIT. This means:

- Executor will boot and run Mac apps, but slowly
- A 68030-era Mac ran at ~25 MHz; the interpreter simulates this much more
  slowly on a modern arm64 core
- For testing old Mac System 6/early System 7 era software (the apps executor
  actually supports), this may be acceptable

**Near term:** Accept interpreter performance. Test with the included skel
volume to confirm basic functionality works.

**Longer term (large project):** Add arm64 JIT to syn68k. This requires:
- Porting the x86 code generation templates in `syn68k/runtime/native/i386/`
  to arm64
- Writing new xlate.h / template.h for AArch64 calling conventions
- Roughly 2-4 weeks of work

---

## Phase 7 — Runtime setup and testing

Once it links:

```bash
# Copy the skel volume to a writable location
cp -Rp /usr/local/share/executor/skel/volume /tmp/ExecutorVolume

# Set required environment variables
export SystemFolder="/tmp/ExecutorVolume/System Folder"

# Run it
./build-sdl/executor
```

Expected issues to resolve at runtime:
- Sound: the SDL2 sound driver uses System V semaphores (`semget`) which exist
  on macOS but may need kernel config; test with `-nosound` first
- Clipboard: `SDL_GetClipboardText`/`SDL_SetClipboardText` are wired up in
  `sdlscrap.c` for macOS; verify round-trip works
- Fullscreen: `SDL_WINDOW_FULLSCREEN_DESKTOP` is set if `SDL_FULLSCREEN` env
  var is set; default is windowed
- Mouse: the macOS event thread model changed in SDL2; verify mouse tracking
  in the emulated Mac UI feels correct

---

## Work order

1. Phase 1 (headers) — unblocks the most files with minimal risk
2. Phase 4 (SDL_bmp.c) — trivial, remove the noise from builds
3. Phase 2 (ast-grep .p fixes) — bulk of the work, do files with most errors first
4. Phase 3 (union arithmetic) — followup after Phase 2
5. Phase 5 (misc) — small cleanup
6. Phase 7 (runtime testing) — verify it actually runs
7. Phase 6 (JIT) — optional, larger project

---

## Quick reference: macro cheat sheet

```c
// Reading from a Handle (HIDDEN_T *h):
STARH(h)              // -> native T*  (translates Mac addr to host ptr)
HxP(h, field)         // PPR(STARH(h)->field)  - read a pointer field
HxX(h, field)         // STARH(h)->field        - read a non-pointer field

// Writing to a Handle:
HPTR_WRITE(h, ptr)    // (h)->pp = RPP(ptr)
HPTR_WRITE0(h)        // (h)->pp = 0

// Reading from a HIDDEN_T value (not pointer):
FROM_HIDDEN(val)      // PPR(val)  - translate to native ptr
HIDDEN_VAL(val)       // val.pp    - raw uint32 Mac address

// Writing to a HIDDEN_T value:
HIDDEN_VAL_WRITE(val, ptr)   // val.pp = RPP(ptr)
HIDDEN_VAL_WRITE0(val)        // val.pp = 0

// Low globals (use the GET_/SET_ macros or aliases once Phase 1 done):
GET_TheZone()         SET_TheZone(v)
GET_SysZone()         SET_SysZone(v)
GET_ApplZone()        SET_ApplZone(v)
// ... etc.
```
