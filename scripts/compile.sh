#!/usr/bin/env bash
set -euo pipefail

# Compile the TeachTiles sketch for ESP32 with PxMatrix enabled and local libraries.
BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ARDUINO_LIBS="$BASE_DIR/arduino_libs,$HOME/Arduino/libraries"

arduino-cli compile --fqbn esp32:esp32:esp32 --libraries "$ARDUINO_LIBS" "$BASE_DIR"
