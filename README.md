# TeachTiles
2025 Senior Capstone Project, for CAA class Honors Engineering Design &amp; Development

## Networking test modes
You can send captured MIDI packets from one ESP32 to another using one of these transports:

- ESP-NOW (recommended for two ESP32 devices on the same channel): low-latency, peer-to-peer.
- UDP (useful for testing on your Mac): ESP32 sends to a UDP port on your laptop where a small Python receiver can display packets.

Setup notes:

- To use ESP-NOW: set `transport = TM_ESPNOW` in `main.cpp` and set `espnow_peer_mac` to the receiver's MAC address (printable via `Serial.println(WiFi.macAddress())`). Re-upload to both devices. The sender will call ESP-NOW and the receiver will print received packets.
- To test with a Mac (single ESP32): set `transport = TM_UDP` in `main.cpp`, run `scripts/udp_receiver.py` on your Mac (`python3 scripts/udp_receiver.py`), then upload the sketch to the ESP32. The ESP32 will send UDP packets to `router_ip`:`router_port` (default 5005).

Notes:

- ESP-NOW requires adding the peer MAC address to the sender's peer list (the code prints a message if peer MAC is not configured).
- UDP is simpler for local testing: both the ESP32 and your Mac must be on the same WiFi network; ensure `router_ip` is set to your Mac's IP in `main.cpp` before compiling.

## Homebrew dependencies

This project uses several system packages (Arduino CLI, SDL2, RtMidi, etc.) which can be installed via Homebrew using the included `Brewfile`.

Install Homebrew (if you don't have it):

```bash
# Follow the instructions at https://brew.sh/ to install Homebrew on macOS
```

Install the packages listed in the repository `Brewfile`:

```bash
brew bundle install --file=./Brewfile
```

If you add or remove Homebrew packages locally, update the `Brewfile` with:

```bash
brew bundle dump --file=./Brewfile --describe --force
```

These commands will ensure reproducible system dependencies for building and testing the project locally.

## External Arduino libraries (keep repo minimal)

The project depends on a few heavy Arduino libraries (for example `PxMatrix` for HUB75 panels and `FastLED` for LED strips). To keep this repository small you have two recommended options:

1) Preferred — install libraries with `arduino-cli` (no repo bloat)

```bash
# Install required Arduino libraries into your local Arduino libraries folder
arduino-cli lib install "PxMatrix"
arduino-cli lib install "FastLED"
```

Arduino CLI will install these into `$(arduino-cli config dump | grep "sketchbook_path" -A1 || echo ~/Arduino)/libraries` (normally `~/Arduino/libraries`). The project's `scripts/compile.sh` and `scripts/upload.sh` now compile using your local Arduino libraries, so the repo doesn't need to include them.

2) Alternative — keep libs as git submodules (explicit, reproducible)

If you need the repo to contain exact versions of each library, use git submodules. Create entries in `.gitmodules` (or copy from `.gitmodules.template`) and add each library as a submodule:

```bash
# Example (replace URL and path with the library repo you want to pin):
git submodule add https://github.com/2dom/PxMatrix.git external_libraries/PxMatrix
git submodule add https://github.com/FastLED/FastLED.git external_libraries/FastLED
git submodule update --init --recursive
```

When using submodules, update `scripts/compile.sh` or `scripts/upload.sh` to include `external_libraries` in the `--libraries` path if you want arduino-cli to prefer them.

Notes and tips
- If you only need to compile locally, prefer the `arduino-cli lib install` approach — it's faster and keeps the repo light.
- If you add submodules, commit the `.gitmodules` file and the submodule pointers so CI can fetch the right versions.
- If you run into space or memory issues on the build server, remove unnecessary submodules or rely on Homebrew-installed packages where possible.

