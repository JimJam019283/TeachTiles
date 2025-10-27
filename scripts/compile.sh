#!/usr/bin/env bash
set -euo pipefail

# Compile the TeachTiles sketch for ESP32 with PxMatrix enabled and local libraries.
BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
arduino-cli compile --fqbn esp32:esp32:esp32 --libraries "$HOME/Arduino/libraries" "$BASE_DIR"
