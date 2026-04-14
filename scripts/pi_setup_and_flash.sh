#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

FQBN="${FQBN:-esp32:esp32:esp32}"
PORT="${PORT:-}"
SKIP_CORE_INSTALL="${SKIP_CORE_INSTALL:-0}"

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "This script is intended to run on Raspberry Pi / Linux."
  exit 1
fi

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli is not installed. Install it first:"
  echo "  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh"
  exit 1
fi

if [[ "$SKIP_CORE_INSTALL" != "1" ]]; then
  echo "Ensuring ESP32 core is installed..."
  arduino-cli core update-index --additional-urls https://dl.espressif.com/dl/package_esp32_index.json
  if ! arduino-cli core list | grep -q '^esp32:esp32'; then
    arduino-cli core install esp32:esp32
  fi
fi

if [[ -z "$PORT" ]]; then
  echo "Detecting ESP32 serial port..."
  PORT="$(arduino-cli board list | awk '/(esp32:esp32|ESP32|ttyUSB|ttyACM)/ {print $1; exit}')"
fi

if [[ -z "$PORT" ]]; then
  for candidate in /dev/ttyUSB0 /dev/ttyACM0 /dev/ttyUSB1 /dev/ttyACM1; do
    if [[ -e "$candidate" ]]; then
      PORT="$candidate"
      break
    fi
  done
fi

if [[ -z "$PORT" ]]; then
  echo "Could not detect ESP32 serial port."
  echo "Run: arduino-cli board list"
  echo "Then rerun with: PORT=/dev/ttyUSB0 ./scripts/pi_setup_and_flash.sh"
  exit 1
fi

echo "Using serial port: $PORT"

echo "Compiling sketch..."
arduino-cli compile --fqbn "$FQBN" "$ROOT"

echo "Uploading sketch..."
arduino-cli upload -p "$PORT" --fqbn "$FQBN" "$ROOT"

echo "Installing/starting bridge service..."
"$ROOT/scripts/install_midi_bridge_pi.sh"

echo

echo "Done. Verify with:"
echo "  sudo systemctl status teachtiles-midi-bridge.service"
echo "  sudo journalctl -u teachtiles-midi-bridge.service -f"
