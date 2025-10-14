#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SKETCH="$ROOT/hardware_test"
# Use bundled arduino-cli if present
if [ -x "$ROOT/arduino-cli_1.3.1_macOS_64bit/arduino-cli" ]; then
  ARDUINO_CLI="$ROOT/arduino-cli_1.3.1_macOS_64bit/arduino-cli"
else
  ARDUINO_CLI=$(command -v arduino-cli || true)
fi
if [ -z "$ARDUINO_CLI" ]; then
  echo "arduino-cli not found. Install it or place bundled binary at $ROOT/arduino-cli_1.3.1_macOS_64bit/arduino-cli"
  exit 1
fi

PORT="${1:-}"
if [ -z "$PORT" ]; then
  candidates=(/dev/cu.SLAB_USBtoUART /dev/cu.usbserial* /dev/cu.usbmodem* /dev/cu.wchusbserial* /dev/cu.Bluetooth-Incoming-Port)
  for c in "${candidates[@]}"; do
    for match in $c; do
      if [ -e "$match" ]; then
        PORT="$match"
        break 2
      fi
    done
  done
fi
if [ -z "$PORT" ]; then
  echo "No serial port specified or detected. Pass it as first arg."
  exit 2
fi

echo "Compiling and uploading Monitor sketch to $PORT"
$ARDUINO_CLI compile --fqbn esp32:esp32:esp32 "$SKETCH"
$ARDUINO_CLI upload -p "$PORT" --fqbn esp32:esp32:esp32 "$SKETCH"

echo "Done. Open serial monitor to view MIDI messages."
