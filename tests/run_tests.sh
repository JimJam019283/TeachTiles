#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUTDIR="$ROOT/build/tests"
mkdir -p "$OUTDIR"

for f in "$ROOT"/tests/test_*.cpp; do
  name=$(basename "$f" .cpp)
  out="$OUTDIR/$name"
  echo "Compiling $name..."
  /usr/bin/g++ -g -std=c++17 -I"$ROOT" -o "$out" "$f" "$ROOT/main.cpp" -pthread
  echo "Running $name..."
  "$out"
done
