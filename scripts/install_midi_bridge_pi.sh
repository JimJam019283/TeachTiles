#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "This installer is for Raspberry Pi / Linux only."
  exit 1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SERVICE_TEMPLATE="$ROOT/scripts/teachtiles-midi-bridge.service"
VENV_DIR="${VENV_DIR:-$ROOT/.venv-bridge}"
PYTHON_BIN="${PYTHON_BIN:-python3}"
SCRIPT_PATH="$ROOT/scripts/midi_bridge_daemon.py"
SERVICE_NAME="teachtiles-midi-bridge.service"
SERVICE_DEST="/etc/systemd/system/$SERVICE_NAME"
RUN_USER="${SUDO_USER:-$USER}"
RUN_GROUP="$(id -gn "$RUN_USER")"

if [[ ! -f "$SCRIPT_PATH" ]]; then
  echo "Bridge script not found: $SCRIPT_PATH"
  exit 1
fi

if ! command -v "$PYTHON_BIN" >/dev/null 2>&1; then
  echo "Python executable not found: $PYTHON_BIN"
  exit 1
fi

if [[ ! -d "$VENV_DIR" ]]; then
  echo "Creating virtual environment at $VENV_DIR"
  "$PYTHON_BIN" -m venv "$VENV_DIR"
fi

VENV_PYTHON="$VENV_DIR/bin/python3"
VENV_PIP="$VENV_DIR/bin/pip"

echo "Installing bridge dependencies into $VENV_DIR"
"$VENV_PIP" install --upgrade pip
"$VENV_PIP" install mido python-rtmidi pyserial

TMP_SERVICE="$(mktemp)"
sed \
  -e "s|__USER__|$RUN_USER|g" \
  -e "s|__WORKDIR__|$ROOT|g" \
  -e "s|__PYTHON__|$VENV_PYTHON|g" \
  -e "s|__SCRIPT__|$SCRIPT_PATH|g" \
  "$SERVICE_TEMPLATE" > "$TMP_SERVICE"

if [[ "${EUID}" -ne 0 ]]; then
  SUDO="sudo"
else
  SUDO=""
fi

echo "Installing systemd service to $SERVICE_DEST"
$SUDO cp "$TMP_SERVICE" "$SERVICE_DEST"
$SUDO chmod 644 "$SERVICE_DEST"
$SUDO systemctl daemon-reload
$SUDO systemctl enable "$SERVICE_NAME"
$SUDO systemctl restart "$SERVICE_NAME"
rm -f "$TMP_SERVICE"

echo
echo "TeachTiles MIDI bridge installed."
echo "Service: $SERVICE_NAME"
echo "Run user: $RUN_USER:$RUN_GROUP"
echo
echo "Useful commands:"
echo "  sudo systemctl status $SERVICE_NAME"
echo "  sudo journalctl -u $SERVICE_NAME -f"
echo "  sudo systemctl restart $SERVICE_NAME"
echo
echo "If serial access fails, ensure $RUN_USER is in the dialout group:"
echo "  sudo usermod -a -G dialout $RUN_USER"
echo "  Then log out and back in."
