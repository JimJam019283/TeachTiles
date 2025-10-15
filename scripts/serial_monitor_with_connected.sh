#!/bin/zsh
# Usage: serial_monitor_with_connected.sh /dev/cu.usbserial-0001 115200
# Prints a clear "Connected" line then streams the serial port.

PORT=${1:-/dev/cu.usbserial-0001}
BAUD=${2:-115200}

if [ ! -e "$PORT" ]; then
  echo "Port $PORT not found" >&2
  exit 1
fi

# Configure the serial port on macOS
stty -f "$PORT" "$BAUD" cs8 -cstopb -parenb -ixon -ixoff -echo 2>/dev/null || true

# Confirm the port is open and then stream
echo "Connected to $PORT at $BAUD"
exec cat "$PORT"
