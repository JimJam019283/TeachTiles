#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
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

PORT="${1:-}" # optional first arg
if [ -z "$PORT" ]; then
  # try to auto-detect common USB serial devices
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
  echo "No serial port specified or detected. Please pass port as first arg, or configure VSCode input 'serialPort'."
  exit 2
fi

ARDUINO_LIBS="$ROOT/arduino_libs,$HOME/Arduino/libraries"

echo "Using arduino-cli: $ARDUINO_CLI"
echo "Compiling sketch (libraries: $ARDUINO_LIBS)"
$ARDUINO_CLI compile --fqbn esp32:esp32:esp32 --libraries "$ARDUINO_LIBS" "$ROOT"

echo "Uploading to port: $PORT"
$ARDUINO_CLI upload -p "$PORT" --fqbn esp32:esp32:esp32 "$ROOT"

echo "Upload finished."
