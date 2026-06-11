#!/usr/bin/env bash
# Run make in build-sdl/ and show only error lines, with a category summary.
set -euo pipefail

REPO="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$REPO/build-sdl"

cd "$BUILD_DIR"
RAW="$(make -j"$(sysctl -n hw.logicalcpu)" 2>&1 || true)"

ERRORS="$(echo "$RAW" | grep " error:" | grep -v "^make\[")"

if [ -z "$ERRORS" ]; then
  echo "No errors."
  exit 0
fi

echo "=== Errors by category ==="
p_count=$(echo "$ERRORS"    | grep -c "no member named 'p'" || true)
arith_count=$(echo "$ERRORS" | grep -c "invalid operands to binary\|incompatible type" || true)
other_count=$(echo "$ERRORS" | grep -cv "no member named 'p'\|invalid operands to binary\|incompatible type" || true)
total=$(echo "$ERRORS" | wc -l | tr -d ' ')

echo "  no member named 'p' (Phase 2): $p_count"
echo "  union arithmetic (Phase 3):    $arith_count"
echo "  other:                         $other_count"
echo "  total:                         $total"
echo ""
echo "=== All errors ==="
echo "$ERRORS"
