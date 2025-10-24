Monalith HUB75 (64x64) setup

This project contains a Monalith visualizer that supports both FastLED (single-wire strips)
and HUB75 parallel panels via the PxMatrix library. The default configuration in
`monalith/src/monalith.cpp` is tuned for a 64x64 HUB75 panel (4096 pixels).

Quick decisions made
- Panel type: HUB75 parallel (64x64, 4096 LEDs)
- Library: PxMatrix (recommended for HUB75 on ESP32)
- Default pin mapping: example pins chosen for a typical ESP32 dev board. Change them to match your wiring.

Recommended pin mapping (change in code or pass defines at compile time):
- LAT: GPIO22
- OE:  GPIO21
- A:   GPIO19
- B:   GPIO18
- C:   GPIO5
- D:   GPIO15

If your panel uses an E address line (for 64px high panels), you may need to wire and configure E as well
and use a PxMatrix constructor that accepts E.

Power
- HUB75 panels are not addressable per-pixel via a single data line and draw a lot of current for bright white.
- Provide a dedicated 5V power supply sized for your panel. For a 64x64 panel, plan for many amps (tens of amps) if you want full white brightness.
- Connect the PSU ground to the ESP32 ground.

Software setup
1. Install libraries:
	- PxMatrix
	- (Optional) FastLED if you use single-wire strips elsewhere

	Example with arduino-cli:

	arduino-cli lib install "PxMatrix"
	arduino-cli lib install "FastLED"

2. Build with PxMatrix support enabled (from the repo root):

	arduino-cli compile --fqbn esp32:esp32:esp32 -DUSE_PXMATRIX /path/to/TeachTiles

	To override pin defaults at compile time, add defines like `-DP_LAT_PIN=22` etc.

3. Upload to your ESP32 (replace `/dev/cu.YOURPORT`):

	arduino-cli upload -p /dev/cu.YOURPORT --fqbn esp32:esp32:esp32 /path/to/TeachTiles

Notes and tuning
- The default code maps MIDI notes across the horizontal axis and places notes vertically centered; chord spreads render to neighboring columns and rows.
- Adjust `WIDTH`, `HEIGHT`, or `noteToIndex()` in `monalith/src/monalith.cpp` if you prefer a different mapping.
- Start with low brightness in `monalith/src/monalith.cpp` or in `matrix.setBrightness()` to avoid excessive current draw during testing.

If you share your exact ESP32 board model and which pins you actually wired, I will pin those values into the code and produce a compile with PxMatrix enabled and report the binary size.

Quick scripts
- `scripts/compile.sh` — compile using local libraries and pinned PxMatrix support
- `scripts/upload.sh /dev/cu.YOURPORT` — compile and upload to the given serial port

These scripts call `arduino-cli` and use the `arduino_libs` folder in the repo plus `~/Arduino/libraries`.
