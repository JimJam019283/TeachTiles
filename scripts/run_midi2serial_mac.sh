#!/bin/bash
# Run MIDI to Serial bridge on Mac
# Usage: ./run_midi2serial_mac.sh

cd "$(dirname "$0")/.."

echo "=========================================="
echo "  TeachTiles MIDI -> Serial Bridge (Mac)"
echo "=========================================="
echo

python3 scripts/midi2serial.py "$@"
