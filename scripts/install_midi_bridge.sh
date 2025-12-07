#!/bin/bash
# Install TeachTiles MIDI Bridge as a persistent background service
# This will automatically start whenever you log in and run continuously

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PLIST_SRC="$SCRIPT_DIR/com.teachtiles.midibridge.plist"
PLIST_DEST="$HOME/Library/LaunchAgents/com.teachtiles.midibridge.plist"
LOG_DIR="$HOME/Library/Logs/TeachTiles"

echo "=========================================="
echo "  TeachTiles MIDI Bridge Installer"
echo "=========================================="
echo

# Create log directory
mkdir -p "$LOG_DIR"

# Stop existing service if running
if launchctl list | grep -q "com.teachtiles.midibridge"; then
    echo "Stopping existing service..."
    launchctl unload "$PLIST_DEST" 2>/dev/null
fi

# Copy plist to LaunchAgents
echo "Installing launch agent..."
cp "$PLIST_SRC" "$PLIST_DEST"

# Load the service
echo "Starting service..."
launchctl load "$PLIST_DEST"

echo
echo "✓ MIDI Bridge installed and started!"
echo
echo "The bridge will:"
echo "  • Start automatically when you log in"
echo "  • Auto-detect your MIDI keyboard and ESP32"
echo "  • Reconnect if devices are unplugged/replugged"
echo
echo "Logs are saved to:"
echo "  $LOG_DIR/midi_bridge.log"
echo
echo "To check status:  launchctl list | grep teachtiles"
echo "To stop:          launchctl unload $PLIST_DEST"
echo "To start:         launchctl load $PLIST_DEST"
echo "To uninstall:     launchctl unload $PLIST_DEST && rm $PLIST_DEST"
echo
