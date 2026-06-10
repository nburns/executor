# Musashi Integration Plan

Replace syn68k with Musashi as Executor's 68k CPU engine. Target: x86_64 macOS only.

## Approach

- Add Musashi as a git submodule at `src/musashi/`
- Write a `syn68k_public.h` shim so the 76+ files that include it need no changes
- Custom musashi config and glue code live in `src/musashi_config/`

## New files

| Path | Purpose |
|---|---|
| `src/musashi/` | Submodule: `https://github.com/kstenerud/Musashi.git` |
| `src/musashi_config/executor_m68kconf.h` | Custom musashi config (68040, instruction hook enabled) |
| `src/musashi_config/musashi_glue.c` | Memory callbacks, init, trap dispatch, cpu_state sync |
| `src/include/syn68k_public.h` | Full syn68k API shim (replaces external libsyn68k header) |

## Modified files

| Path | Change |
|---|---|
| `src/configure.ac` | Remove `AC_CHECK_LIB(syn68k, ...)` error |
| `src/Makefile.am` | Extend `CONFIG_ARCH_X86_64` block (see below) |

## Makefile.am changes (CONFIG_ARCH_X86_64 block)

```makefile
if CONFIG_ARCH_X86_64

CLEANFILES += musashi/m68kops.c musashi/m68kops.h musashi/m68kmake

musashi/m68kmake: musashi/m68kmake.c
	$(CC) -o musashi/m68kmake musashi/m68kmake.c

musashi/m68kops.c musashi/m68kops.h: musashi/m68kmake
	cd musashi && ./m68kmake

arch_sources = \
    config/arch/x86_64/x86_64.c        \
    config/arch/x86_64/x86_64.h        \
    musashi/m68kcpu.c                   \
    musashi/m68kdasm.c                  \
    musashi/softfloat/softfloat.c       \
    musashi_config/musashi_glue.c       \
    musashi_config/executor_m68kconf.h

nodist_executor_SOURCES += musashi/m68kops.c

AM_CPPFLAGS += \
    -I$(srcdir)/musashi            \
    -I$(srcdir)/musashi_config     \
    -DMUSASHI_CNF=\"executor_m68kconf.h\"

endif CONFIG_ARCH_X86_64
```

## syn68k_public.h shim

Defines the entire syn68k API surface so no other source file needs changing:

- `syn68k_addr_t` — `uint32_t`
- `SYN68K_TO_US(addr)` / `US_TO_SYN68K(ptr)` — pointer arithmetic on `ROMlib_emulator_memory`
- `SYN68K_TO_US_CHECK0` / `US_TO_SYN68K_CHECK0` — null-safe variants
- `cpu_state` — shadow struct (see below); synced with Musashi at callback boundaries
- `EM_D0`..`EM_D7`, `EM_A0`..`EM_A7` — map to `cpu_state.regs[]`
- `PUSHUL`, `POPUL`, `PUSHUW`, `POPUW`, `PUSHADDR`, `POPADDR` — stack macros using `EM_A7`
- `READUW`, `READUL` — direct reads from emulator memory
- `CALL_EMULATOR(addr)` — sync cpu_state to Musashi, set PC, call `m68k_execute`, sync back
- `callback_handler_t`, `callback_argument()` — callback plumbing
- `FAKEPascalToCCall` stub

## cpu_state layout

```c
typedef union { int32_t n; } ul_t;
typedef union { uint16_t n; } uw_t;
typedef union { int16_t n; } sw_t;
typedef union { ul_t ul; uw_t uw; sw_t sw; } reg_t;

struct {
    reg_t regs[16];   // D0-D7 at [0]-[7], A0-A7 at [8]-[15]
    int8_t ccn, ccnz, ccc, ccv, ccx;
    int interrupt_pending[8];
    int interrupt_status_changed;
} cpu_state;
```

Synced via `cpu_state_from_musashi()` / `cpu_state_to_musashi()` in `musashi_glue.c` using
`m68k_get_reg` / `m68k_set_reg`. Called on every entry/exit to a trap handler and around
`CALL_EMULATOR`.

## musashi_glue.c responsibilities

- `uint8_t *ROMlib_emulator_memory` — the flat 68k address space buffer
- `m68k_read_memory_8/16/32`, `m68k_write_memory_8/16/32` — Musashi memory callbacks; no
  byte-swapping (Musashi emulates BE internally; buffer stores data in BE order as written by
  Mac code)
- `initialize_68k_emulator(...)` — receives `(uint32_t *) SYN68K_TO_US(0)` as the memory
  pointer; calls `m68k_init()`, `m68k_set_cpu_type(M68K_CPU_TYPE_68040)`, `m68k_pulse_reset()`
- `trap_install_handler(trapnum, fn, data)` — stores A-line handler
- A-line dispatch — via `M68K_ILLG_HAS_CALLBACK` (Musashi routes 1010 opcodes as illegal
  instructions); handler calls stored `aline_handler`, sets PC to return value
- Interrupt loop — after each `m68k_execute` slice, check `interrupt_status_changed` and call
  `m68k_set_irq` for the highest pending level

## executor_m68kconf.h

- `M68K_EMULATE_040 M68K_OPT_ON`
- `M68K_ILLG_HAS_CALLBACK M68K_OPT_SPECIFY_HANDLER` (for A-line dispatch)
- `M68K_EMULATE_INT_ACK M68K_OPT_ON`
- Prefetch and address error off

## CALL_EMULATOR

Callers push a sentinel return address (`0x88A84321` or similar) before calling
`CALL_EMULATOR`. The instruction hook detects when PC reaches the sentinel and calls
`m68k_end_timeslice()` to break out of the inner `m68k_execute`. Musashi supports recursive
`m68k_execute` calls from within callbacks.

## Build sequence

```sh
git submodule update --init
cd src && autoreconf --install
mkdir ../build && cd ../build
../src/configure
make
```

## Key risk

Musashi routes 1010 (A-line) opcodes through its illegal-instruction path. Need to verify that
`M68K_ILLG_HAS_CALLBACK` fires for these and that returning 1 from the callback allows us to
set the PC to the handler's return value cleanly. Prototype this first before investing in the
rest of the glue.
