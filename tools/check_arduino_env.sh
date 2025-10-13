#!/usr/bin/env bash
# Simple helper to check Arduino CLI and esp32 core presence
set -euo pipefail

echo "Checking for arduino-cli..."
if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli not found. Install via Homebrew: brew install arduino-cli"
  exit 1
fi

echo "arduino-cli found: $(command -v arduino-cli)"

echo "Listing installed cores (filtered for esp32):"
arduino-cli core list | grep esp32 || true

if arduino-cli core list | grep -q "esp32:esp32"; then
  echo "ESP32 core appears installed."
else
  echo "ESP32 core not found. Install it with:"
  echo "  arduino-cli core update-index --additional-urls https://dl.espressif.com/dl/package_esp32_index.json"
  echo "  arduino-cli core install esp32:esp32"
  exit 2
fi

echo "Environment looks okay. You can compile with:"
echo "  arduino-cli compile --fqbn esp32:esp32:esp32 $(pwd)"
