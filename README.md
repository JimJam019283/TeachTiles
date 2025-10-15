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
