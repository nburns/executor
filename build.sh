#!/usr/bin/env bash
set -euo pipefail

REPO="$(cd "$(dirname "$0")" && pwd)"
SYN68K_DIR="$REPO/syn68k"
SYN68K_PREFIX="$REPO/syn68k-prefix"
BUILD_DIR="$REPO/build"

# Homebrew libtoolize ships as glibtoolize on macOS
export PATH="/opt/homebrew/bin:$PATH"
if ! command -v libtoolize &>/dev/null && command -v glibtoolize &>/dev/null; then
  export LIBTOOLIZE=glibtoolize
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
  SDL2_CFLAGS="$(sdl2-config --cflags)"
  SDL2_LIBS="$(sdl2-config --libs)"
  CPPFLAGS="-I$SYN68K_PREFIX/include $SDL2_CFLAGS" \
  LDFLAGS="-L$SYN68K_PREFIX/lib $SDL2_LIBS" \
    "$REPO/src/configure"
fi

make -j"$(sysctl -n hw.logicalcpu)"

echo "==> Done. Binary: $BUILD_DIR/executor"
