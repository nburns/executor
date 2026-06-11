#!/usr/bin/env bash
set -euo pipefail

REPO="$(cd "$(dirname "$0")" && pwd)"
SYN68K_DIR="$REPO/syn68k"
SYN68K_PREFIX="$REPO/syn68k-prefix"
BUILD_DIR="$REPO/build-sdl"

# Homebrew libtoolize ships as glibtoolize on macOS
export PATH="/opt/homebrew/bin:$PATH"
if ! command -v libtoolize &>/dev/null && command -v glibtoolize &>/dev/null; then
  export LIBTOOLIZE=glibtoolize
fi

if command -v pkg-config &>/dev/null && pkg-config --exists sdl2 2>/dev/null; then
  SDL2_CFLAGS="$(pkg-config sdl2 --cflags)"
  SDL2_LIBS="$(pkg-config sdl2 --libs)"
elif command -v sdl2-config &>/dev/null; then
  # sdl2-config --cflags gives -I.../include/SDL2 but code uses <SDL2/SDL.h>,
  # so derive the parent include directory instead
  SDL2_PREFIX="$(sdl2-config --prefix)"
  SDL2_CFLAGS="-I${SDL2_PREFIX}/include $(sdl2-config --cflags | sed 's|-I[^ ]*/SDL2 *||g')"
  SDL2_LIBS="$(sdl2-config --libs)"
else
  echo "error: SDL2 not found - install it (e.g. brew install sdl2)" >&2
  exit 1
fi

# ── syn68k ────────────────────────────────────────────────────────────────────

echo "==> Building syn68k..."
cd "$SYN68K_DIR"

if [ ! -f configure ]; then
  if [ -n "${LIBTOOLIZE:-}" ]; then
    "$LIBTOOLIZE"
    aclocal
    autoheader
    automake --add-missing
    autoconf
  else
    bash autogen.sh
  fi
fi

mkdir -p "$SYN68K_DIR/build"
cd "$SYN68K_DIR/build"

if [ ! -f Makefile ]; then
  ../configure --prefix="$SYN68K_PREFIX"
fi

make -j"$(sysctl -n hw.logicalcpu)"
make install

# ── executor ──────────────────────────────────────────────────────────────────

echo "==> Building executor..."
cd "$REPO/src"

if [ ! -f configure ]; then
  autoreconf --install
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f Makefile ]; then
  CPPFLAGS="-I$SYN68K_PREFIX/include $SDL2_CFLAGS" \
  LDFLAGS="-L$SYN68K_PREFIX/lib $SDL2_LIBS" \
    "$REPO/src/configure" --with-front-end=sdl
fi

MAKE_CMD="make -j$(sysctl -n hw.logicalcpu)"
if command -v bear &>/dev/null; then
  bear -- $MAKE_CMD
else
  $MAKE_CMD
fi

echo "==> Done. Binary: $BUILD_DIR/executor"
if ! command -v bear &>/dev/null; then
  echo "    (install bear via 'brew install bear' to also generate compile_commands.json)"
fi
