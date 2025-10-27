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
  /usr/bin/g++ -g -std=c++17 -I"$ROOT" -I"$ROOT/monalith" -DTEST_RUNNER -o "$out" "$f" "$ROOT/main.cpp" "$ROOT/tests/helpers_transport.cpp" "$ROOT/monalith/monalith.cpp" -pthread
  echo "Running $name..."
  "$out"
done
