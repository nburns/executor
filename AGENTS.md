# Executor — Agent Guide

## What this is

Executor is a clean-room reimplementation of the classic Mac Toolbox (System 6 / early System 7 era) that can run 680x0 Mac binaries on modern hosts. The active goal is getting it to **compile and run on arm64 macOS** with an SDL2 front-end.

## Build

```bash
# Dependencies (one-time)
brew install sdl2 autoconf automake libtool bear

# Build syn68k + executor (produces build-sdl/executor)
./build.sh
```

If `build-sdl/Makefile` already exists, `build.sh` skips configure and goes straight to `make`. To force a reconfigure (e.g. after changing `configure.ac`), delete `build-sdl/Makefile` first.

If `bear` is installed, `build.sh` automatically generates `build-sdl/compile_commands.json` during the build. This is needed for accurate clangd and ast-grep usage — install it.

## Check current errors

```bash
./errors.sh          # build and show categorized error summary
```

Or manually:
```bash
cd build-sdl && make 2>&1 | grep " error:" | grep -v "^make\["
```

## Current status

Phases 1, 2, and 4 are done. Remaining work is Phase 3 → 5 → 7 (runtime).

| Phase | What | Status |
|---|---|---|
| 1 | Header GET_/SET_ aliases for undeclared low globals | ✓ Done |
| 2 | Direct `.p` member access on HIDDEN_* types | ✓ Done |
| 3 | Header macro aliases + union arithmetic | **Next** |
| 4 | SDL_bmp.c SDL2 API fixes | ✓ Done |
| 5 | Misc: alias.c, dump.c | After Phase 3 |
| 7 | Runtime testing with skel volume | After Phase 5 |

Current error count: ~74 (up from 54 before Phase 2 — fixing Phase 2 source errors unmasked Phase 3 header macro errors that were previously hidden by clang's 20-error-per-TU limit).

Run `./errors.sh` from the repo root to see current errors grouped by category.

## The HIDDEN_* pointer system (root cause of all Phase 2/3 errors)

Executor stores Mac 32-bit addresses in `HIDDEN_*` union types so the same source compiles on both 32-bit and 64-bit hosts.

On **32-bit** (e.g. i386), Mac addresses equal host addresses, so the union has a `.p` field that is a real native pointer:
```c
typedef struct { SomeType *p; } HIDDEN_SomeType;
```

On **64-bit** (arm64, `SIZEOF_CHAR_P == 8`), the union stores only the 32-bit Mac address and requires translation to reach host memory:
```c
typedef union { uint32_t pp; SomeType *type[0]; } HIDDEN_SomeType;
```

The `.p` field **does not exist** on 64-bit. Any code that accesses `.p` directly fails to compile. The accessor macros in `byteswap.h` bridge the gap — they exist for both builds and do the right thing.

`FORCE_EXPERIMENTAL_PACKED_MACROS` (now auto-enabled by configure when `sizeof(char*)==8`) switches which form is compiled. The `configure.ac` change ensures arm64 gets the 64-bit union form automatically.

## Phase 2 substitution rules

These are the mechanical replacements. Always check context — some files have `.p` fields on non-HIDDEN types that must not be touched.

| Old (32-bit) | New (64-bit) | Notes |
|---|---|---|
| `MR(h->p)` | `STARH(h)` | h is HIDDEN_T * |
| `h->p` (read) | `STARH(h)` | |
| `h->p = RM(expr)` | `HPTR_WRITE(h, expr)` | drop the RM |
| `h->p = NULL` or `CLC(0)` | `HPTR_WRITE0(h)` | |
| `MR(val.p)` | `FROM_HIDDEN(val)` | val is HIDDEN_T value (not pointer) |
| `val.p` (read as pointer) | `FROM_HIDDEN(val)` | |
| `val.p` (read as Mac uint32) | `HIDDEN_VAL(val)` | |
| `val.p = RM(expr)` | `HIDDEN_VAL_WRITE(val, expr)` | drop the RM |
| `val.p = 0` or `CLC(0)` or `NULL` | `HIDDEN_VAL_WRITE0(val)` | |
| `sizeof(someHidden.p)` | `sizeof(Ptr)` | only known instance: main.c:1939 |

## Phase 3 substitution rules

Union arithmetic errors: `HIDDEN_*` value used in a comparison or arithmetic expression.

```c
// Before
if (someHandle->field == 0)

// After (NULL check on pointer field)
if (HxZ(someHandle, field) == 0)
// or
if (HxP(someHandle, field) == NULL)
```

Use `HIDDEN_VAL(x)` to extract the raw uint32 for integer comparisons; `PPR(x)` to get the translated native pointer.

## Macro quick reference

```c
// Reading via a Handle (HIDDEN_T *h):
STARH(h)           // translate h->pp to native T*
HxP(h, field)      // PPR(STARH(h)->field)  - read a pointer field
HxX(h, field)      // STARH(h)->field        - read a non-pointer field
HxZ(h, field)      // test field for zero without full translation

// Writing via a Handle:
HPTR_WRITE(h, ptr)    // h->pp = RPP(ptr)
HPTR_WRITE0(h)        // h->pp = 0

// Reading a HIDDEN_T value (not pointer-to-hidden):
FROM_HIDDEN(val)      // PPR(val) - translate to native ptr
HIDDEN_VAL(val)       // val.pp   - raw uint32 Mac address

// Writing a HIDDEN_T value:
HIDDEN_VAL_WRITE(val, ptr)  // val.pp = RPP(ptr)
HIDDEN_VAL_WRITE0(val)      // val.pp = 0

// Low globals (set by Phase 1):
GET_TheZone()    SET_TheZone(v)
GET_SysZone()    SET_SysZone(v)
GET_ApplZone()   SET_ApplZone(v)
// ... see MemoryMgr.h, WindowMgr.h, FileMgr.h, ResourceMgr.h, DialogMgr.h
```

## Phase 2 failing files (23 files, sorted by error count)

```
launch.c (13)    executor.c (9)   main.c (8)       osutil.c (8)
toolevent.c (7)  toolutil.c (7)   segment.c (7)    syserr.c (7)
scrap.c (6)      device.c (5)     font.c (4)       stdfile.c (4)
osevent.c (3)    mmansubr.c (3)   mman.c (3)       desk.c (3)
emustubs.c (3)   system_error.c (3) icon.c (3)     appearance.c (2)
aboutbox.c (1)   crc.c (1)        iu.c (1)
```

Start with `launch.c` and `executor.c` — they have the most errors and are central to the boot path.

## CAUTION: non-HIDDEN `.p` fields

`.p` exists on other types in this codebase — do NOT apply rewrites globally:
- `obj->u.cfdib.p` in `SDL_bmp.c`
- `rp.p` in `qStdLine.c`
- Various struct fields unrelated to HIDDEN_*

Scope all ast-grep rewrites to the 23 files listed above, and verify each substitution makes sense in context before applying.

## Tooling

- **ast-grep**: use for structural search and rewrite (`sg --pattern` / `sg --rewrite`). Needs `compile_commands.json` for accurate type-aware matching.
- **clangd**: use via LSP for go-to-definition, find-references, hover types. Needs `compile_commands.json`.
- **rg**: use for plain text search (strings, comments). Not for code structure.
- Do NOT use grep/sed for code edits — they miss scope and will produce wrong substitutions on the `.p` disambiguation problem.

Key headers to understand before editing:
- `src/include/rsys/byteswap.h` — all the HIDDEN_* macros and MR/RM/PPR/RPP
- `src/include/MacTypes.h` — MAKE_HIDDEN macro definition
- `src/include/rsys/lowglobals.h` — low-memory global declarations
