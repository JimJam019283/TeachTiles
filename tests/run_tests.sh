#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUTDIR="$ROOT/build/tests"
mkdir -p "$OUTDIR"

for f in "$ROOT"/tests/test_*.cpp; do
  name=$(basename "$f" .cpp)
  out="$OUTDIR/$name"
  echo "Compiling $name..."
  # Link the test with main.cpp and test helper that defines host-side symbols
  # Disable Monalith to avoid hardware dependencies during host-side testing
  # Include example_bitmap.c for the symbol reference in main.cpp
  /usr/bin/g++ -g -std=c++17 -I"$ROOT" -I"$ROOT/monalith" -DTEST_RUNNER -DENABLE_MONALITH=0 -o "$out" "$f" "$ROOT/main.cpp" "$ROOT/tests/helpers_transport.cpp" "$ROOT/example_bitmap.c" -pthread
  echo "Running $name..."
  "$out"
done
