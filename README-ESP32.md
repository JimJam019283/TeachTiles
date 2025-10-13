ESP32 build & install guide
===========================

This project uses the Arduino framework for the ESP32. Your system needs `arduino-cli` (or PlatformIO) and the Espressif ESP32 core installed to compile `main.cpp` for the device.

Quick steps (macOS, zsh)
------------------------

1) Install Arduino CLI (Homebrew):

```bash
brew install arduino-cli
```

2) Add Espressif core index (if needed) and install the ESP32 core:

```bash
arduino-cli core update-index --additional-urls https://dl.espressif.com/dl/package_esp32_index.json
arduino-cli core install esp32:esp32
```

3) Compile the sketch for the ESP32 WROOM (example fully-qualified board name):

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 /path/to/TeachTiles
```

4) Upload to device (replace /dev/tty.SERIAL with your serial port):

```bash
arduino-cli upload -p /dev/tty.SERIAL --fqbn esp32:esp32:esp32 /path/to/TeachTiles
```

Notes
-----
- If you prefer VS Code + PlatformIO, create a PlatformIO project for an ESP32 board and copy `main.cpp` there.
- The repository contains `src/host_main.cpp` and `src/native_main.cpp` for local host testing; these are compiled with your system compiler and simulate Bluetooth/Serial behavior.

If you run into errors, copy/paste the terminal output and I will help diagnose.

Optional: add a VS Code keyboard shortcut to upload (one-step)
---------------------------------------------------------
You can bind a key to the `arduino:upload:auto` task so pressing the shortcut will compile and upload.

1. Open Command Palette (Cmd+Shift+P) -> "Preferences: Open Keyboard Shortcuts (JSON)".
2. Paste the following JSON entry into the array (adjust the key for your platform):

```json
{
	"key": "cmd+alt+u",
	"command": "workbench.action.tasks.runTask",
	"args": "arduino:upload:auto",
	"when": "editorTextFocus"
}
```

- On Windows/Linux you may prefer `ctrl+alt+u` instead of `cmd+alt+u`.
- After saving, press the shortcut to run the upload task. If the task needs a serial port it will attempt to auto-detect one; you can also use the prompt-based `arduino:upload` task and supply a port.

