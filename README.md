# Executor

Executor is a Macintosh emulator capable of running 680x0 Mac binaries (System 6 era, early System 7) without using any intellectual property from Apple. It was originally developed by Abacus Research and Development, Inc. (ARDI) and has not been actively maintained since the mid-2000s. The codebase is preserved here and is currently being ported to arm64/SDL2.

## Overview

Executor translates 68k Mac ROM traps into equivalent native operations. Rather than loading a real Mac ROM, it ships its own clean-room reimplementation of the classic Mac Toolbox — the full stack of managers that Mac applications called into: QuickDraw, Memory Manager, Resource Manager, File Manager, Window Manager, Menu Manager, TextEdit, Sound Manager, and dozens more.

The 68k CPU itself is handled by **syn68k**, a separate subproject that either JIT-compiles 68k instruction blocks to native x86 machine code or falls back to a portable interpreter on other architectures.

## Repository Layout

```
executor/
├── src/            - Executor source (Toolbox reimplementation + front-ends)
├── syn68k/         - 68k CPU emulator subproject (see syn68k/README)
├── syn68k-prefix/  - Local install prefix for syn68k headers/lib
├── build/          - Default autoconf build directory
├── build-sdl/      - SDL2/arm64 build directory (current porting target)
├── build.sh        - One-shot build script (syn68k then executor)
├── docs/           - Historical documentation (FAQs, release notes)
├── configs/        - Platform-specific build configuration fragments
├── system/         - Mac system folder skeleton (skel volume)
├── util/           - Host-side utility programs (makehfv, etc.)
├── icons/          - Application icons
├── splash/         - Splash screen assets
└── lib/            - Third-party libraries used by some ports
```

## Source Tree (`src/`)

### Toolbox Managers

The Mac Toolbox is split into manager-level modules following the original Inside Macintosh naming conventions. Each manager is one or more `<name>.c` files implementing the traps that Mac applications called via the A-trap dispatch mechanism.

| Module group | Files | What it implements |
|---|---|---|
| **QuickDraw** | `q*.c` (~40 files) | All drawing primitives: ports, regions, polygons, pictures, color, GWorlds, cursors, blitters |
| **Memory Manager** | `mman.c`, `mmansubr.c`, `tempmem.c` | Mac heap zones, handles, pointers, temporary memory |
| **Resource Manager** | `res*.c` (~12 files) | Resource forks, resource maps, loading, modification |
| **File Manager** | `file*.c`, `hfs*.c` (~20 files) | HFS filesystem implementation, file I/O, volume management |
| **Window Manager** | `wind*.c` | Window creation, updates, dragging, resizing, z-order |
| **Menu Manager** | `menu.c`, `menuColor.c`, `menuV.c` | Pull-down menus, menu bar, MBDF dispatch |
| **Control Manager** | `ctl*.c` | Buttons, sliders, scrollbars, CDEF dispatch |
| **Dialog Manager** | `dial.c` | Alerts and dialogs, DITL handling |
| **TextEdit** | `te*.c` | Text insertion point, editing, display, scrap |
| **Font Manager** | `font.c`, `fontIMVI.c` | Font loading, glyph rendering, FOND resources |
| **Sound Manager** | `sound.c`, `soundIMVI.c`, `sounddriver.c` | Sound channel dispatch, driver abstraction |
| **Event Manager** | `toolevent.c`, `osevent.c` | Toolbox and OS event queues, WaitNextEvent |
| **Process/Segment** | `launch.c`, `segment.c`, `cfm.c` | Application launch, CODE segment loading, CFM/PEF support |
| **Apple Events** | `AE.c`, `AE_coercion.c`, `AE_desc.c`, `AE_hdlr.c` | Inter-application messaging |
| **Misc managers** | `osutil.c`, `time.c`, `vbl.c`, `syncint.c`, `desk.c`, `scrap.c`, etc. | OS utilities, timers, VBL tasks, Desk Manager, clipboard |

### Trap Dispatch

The A-trap dispatch layer lives in `emustubs.c`, `emutrap.c`, and `emutraptables.c`. When syn68k encounters a Mac A-trap instruction, it vectors through these tables to the appropriate ROMlib implementation. `trapname.c` provides debug name lookup for all ~1000 traps. `interfacelib.c` is the largest single file (~4200 lines) and implements the CFM InterfaceLib stubs used by PowerPC-era applications.

### Low Globals and Memory Layout

Mac applications access a fixed region of low memory (addresses `0x0000`–`0x0E00`) for global state: `TheZone`, `ScrnBase`, `CurStackBase`, and ~200 others. On 32-bit hosts these are mapped directly; on 64-bit hosts they go through GET_/SET_ accessor macros defined in the `HIDDEN_*` union layer (`byteswap.h`, `MacTypes.h`). The memory layout is described in `rsys/memory_layout.h`.

### Internal Headers (`src/include/rsys/`)

Internal headers under `rsys/` define the private interfaces between Executor subsystems. Key files:

- `byteswap.h` — byte-swap macros (`CW`, `CL`), Mac/ROMlib pointer translation (`MR`/`RM`), and the `HIDDEN_*` union types for 64-bit pointer packing
- `vdriver.h` — abstract video driver interface (implemented per front-end)
- `lowglobals.h` — declarations of all Mac low-memory globals
- `common.h` — universal include pulled in by every source file
- `trapglue.h` / `trapdefines.h` — trap number constants and glue

### Front-Ends (`src/config/front-ends/`)

The display, input, and windowing layer is abstracted behind the `vdriver` interface. Available front-ends:

| Front-end | Directory | Status |
|---|---|---|
| **SDL2** | `config/front-ends/sdl/` | Active — current porting target (arm64 + macOS) |
| **X11** | `config/front-ends/x/` | Legacy Linux/Unix |
| **svgalib** | `config/front-ends/svgalib/` | Legacy bare Linux |
| **DOS/DJGPP** | `config/front-ends/dos/`, `djgpp/` | Legacy |
| **Win32** | `config/front-ends/win32/` | Legacy |
| **NeXTSTEP** | `config/front-ends/nextstep/` | Legacy |

The SDL2 front-end (`sdlevents.c`, `sdlwm.c`, `sdlX.c`, `sdlwin.c`, `sdlscrap.c`) handles event translation, window management, and clipboard. Sound uses a parallel pluggable driver system (`config/sound/sdl/sdl-sound.c`).

### Architecture / OS Config (`src/config/arch/`, `src/config/os/`, `src/config/hosts/`)

Per-platform config headers select endianness, pointer width, and platform-specific behaviors. Supported architecture targets include `i386`, `arm64`, `alpha`, `powerpc`, and `x86_64`; OS targets include `linux`, `macosx`, `cygwin32`, `msdos`, and `next`.

## syn68k

syn68k is the 68k CPU emulation engine, built as a separate static library. It is covered in its own README (`syn68k/README`). At a high level:

- On x86, it JIT-compiles blocks of 68k instructions to native x86 machine code using a table-driven code generator (`syngen/`, `runtime/native/i386/`)
- On arm64 and other non-x86 hosts it falls back to the portable `native/null` interpreter
- The library exposes a small public API (`syn68k_public.h`) through which Executor installs trap callbacks, provides a memory accessor, and drives the CPU

The arm64 port currently uses the interpreter backend — there is no arm64 JIT yet. See `plan.md` for the in-progress work to compile Executor on arm64.

## Building

### Quick start (macOS, arm64 or x86)

```bash
# Dependencies (Homebrew)
brew install sdl2 autoconf automake libtool

./build.sh
```

This builds syn68k first (installing into `syn68k-prefix/`), then configures and builds Executor against it. The resulting binary is `build/executor`.

### Manual build

```bash
# 1. Build syn68k
cd syn68k
bash autogen.sh
mkdir build && cd build
../configure --prefix="$(pwd)/../../syn68k-prefix"
make && make install

# 2. Build executor
cd ../../src
autoreconf --install
cd ../build
../src/configure \
  CPPFLAGS="-I../syn68k-prefix/include $(sdl2-config --cflags)" \
  LDFLAGS="-L../syn68k-prefix/lib $(sdl2-config --libs)"
make
```

### Running

```bash
cp -Rp /usr/local/share/executor/skel/volume /tmp/ExecutorVolume
export SystemFolder="/tmp/ExecutorVolume/System Folder"
./build/executor
```

## Current Status

The SDL2/arm64 port is in progress. Phase 1 (64-bit low-global accessor macros) is complete. Remaining work is tracked in `plan.md` and involves converting direct `.p` member accesses on `HIDDEN_*` union types throughout the source to the portable accessor macros (`STARH`, `HPTR_WRITE`, `FROM_HIDDEN`, etc.) required on 64-bit hosts.

## History

Executor was a commercial product sold by ARDI in the 1990s. The source was released to GitHub by the original author (Cliff Matthews, `ctm` on GitHub) after ARDI closed. The codebase predates C99 and uses autoconf/automake as its build system.
