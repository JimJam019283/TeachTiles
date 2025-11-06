// main.cpp
// ESP32 MIDI Note Reader and Router
// This program reads MIDI signals from a digital piano via UART,
// extracts note-on and note-off events (note number and duration),
// and packages the data to send to a router (e.g., via UDP).
// Designed for ESP32 WROOM module using Arduino framework.

// Include Arduino headers only when building for Arduino/ESP32
#if defined(ARDUINO) || defined(ESP32)
#include <Arduino.h>
#define USE_BT 0 // set to 1 to enable BluetoothSerial (increase flash size)
#if USE_BT
#include "BluetoothSerial.h"
#endif
#include <stdint.h>
#else
// Host (non-Arduino) build: provide minimal stubs so the file can be compiled/run locally
#include <cstdint>
#include <chrono>
#include <cstdio>
#include <cstdarg>
#include <thread>

struct DummySerial {
    void begin(int) {}
    /* allow non-literal format strings in host print/printf stubs */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    void println(const char* s) { std::puts(s); }
    void print(const char* s) { std::fputs(s, stdout); }
    void printf(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); }
#pragma GCC diagnostic pop
};
DummySerial Serial;


#include "src/host_stubs.h"
// Host stubs for Serial2 (MIDI UART) and SerialBT (Bluetooth SPP)
#include <deque>
#include <vector>
#include <array>

// Implementations for host stubs declared in src/host_stubs.h
// HostSerial2
static std::deque<uint8_t> __host_serial2_q;
void HostSerial2::begin(int, int, int, int) {}
int HostSerial2::available() { return (int)__host_serial2_q.size(); }
uint8_t HostSerial2::read() { uint8_t v = 0; if (!__host_serial2_q.empty()) { v = __host_serial2_q.front(); __host_serial2_q.pop_front(); } return v; }
void HostSerial2::push(uint8_t b) { __host_serial2_q.push_back(b); }
void HostSerial2::push(const std::vector<uint8_t>& bytes) { for (auto b: bytes) __host_serial2_q.push_back(b); }
HostSerial2 Serial2;

// HostBT
static std::vector<std::array<uint8_t,5>> __host_bt_captured;
bool HostBT::begin(const char* name) { std::printf("(host) Simulated BT start: %s\n", name); return true; }
size_t HostBT::write(const uint8_t* buf, size_t n) {
    std::array<uint8_t,5> pkt = {0,0,0,0,0};
    for (size_t i = 0; i < n && i < pkt.size(); ++i) pkt[i] = buf[i];
    __host_bt_captured.push_back(pkt);
    std::printf("(host) BT packet (%zu bytes) captured\n", n);
    return n;
}
const std::vector<std::array<uint8_t,5>>& HostBT::getCaptured() const { return __host_bt_captured; }
void HostBT::clear() { __host_bt_captured.clear(); }
HostBT SerialBT;

// millis() stub for host
uint32_t millis() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}
#endif

#if !defined(ESP32) && !defined(TEST_RUNNER)
// Minimal host main so local builds/linking succeed. Calls setup() and a few loop() iterations.
// Forward-declare Arduino-style functions so host main can call them before they're defined.
void setup();
void loop();
int main() {
    Serial.begin(115200);
    setup();

    // --- Host simulation: send a Note ON, hold briefly, then Note OFF ---
    Serial.printf("(host) Starting MIDI simulation\n");
    // Note ON: status 0x90 (channel 0), note 60 (C4), velocity 100
    Serial2.push(std::vector<uint8_t>{0x90, 60, 100});
    // Run loop for ~1s (100 iterations * 10ms) to simulate holding the key
    for (int i = 0; i < 100; ++i) {
        loop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    // Note OFF: status 0x80 (channel 0), note 60, velocity 0
    Serial2.push(std::vector<uint8_t>{0x80, 60, 0});
    // Allow some time for the OFF to be processed
    for (int i = 0; i < 50; ++i) {
        loop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Print captured BT/transport packets (host fallback)
    try {
        const auto& captured = SerialBT.getCaptured();
        Serial.printf("(host) Captured %zu packets\n", captured.size());
        for (size_t i = 0; i < captured.size(); ++i) {
            const auto& pkt = captured[i];
            Serial.printf("(host) pkt %zu: %u %u %u %u %u\n", i, pkt[0], pkt[1], pkt[2], pkt[3], pkt[4]);
        }
    } catch (...) {
        Serial.println("(host) No captured packets or SerialBT unavailable");
    }
    return 0;
}
#endif

#include "src/teachtiles.h"
#include "monalith/monalith.h"

#if defined(ESP32)
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
WiFiUDP _udp;

// Peer MAC for ESP-NOW: we can auto-select the peer by matching our own MAC
// If you know both MACs in advance, list them here. The code will pick the other one as peer.
const uint8_t KNOWN_MAC_A[6] = {0xF4,0x65,0x0B,0xC2,0x4A,0x18}; // device seen on /dev/cu.usbserial-0001
const uint8_t KNOWN_MAC_B[6] = {0x4C,0xC3,0x82,0x07,0xCC,0x04}; // device seen on /dev/cu.usbserial-11
uint8_t espnow_peer_mac[6] = {0,0,0,0,0,0};

// Define transport selection default (can be changed at runtime)
TransportMode transport = TM_ESPNOW;

void onDataRecv(const esp_now_recv_info_t* info, const uint8_t *data, int len) {
    if (len < 5) return;
    uint8_t note = data[0];
    uint32_t duration = ((uint32_t)data[1]<<24) | ((uint32_t)data[2]<<16) | ((uint32_t)data[3]<<8) | (uint32_t)data[4];
    Serial.printf("(ESP-NOW RX) Note: %s (%d) | Duration: %lu ms\n", midiNoteToName(note), note, duration);
    // Forward to visualizer
    Monalith::showNote(note, duration);
}

void initEspNow() {
    WiFi.mode(WIFI_STA);
    esp_wifi_set_promiscuous(false);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(onDataRecv);
    // Auto-select peer: if espnow_peer_mac is zero, compare our MAC to known list
    if (memcmp(espnow_peer_mac, (uint8_t[]){0,0,0,0,0,0}, 6) == 0) {
        String macstr = WiFi.macAddress();
        uint8_t mymac[6] = {0};
        int vals[6] = {0};
        if (sscanf(macstr.c_str(), "%x:%x:%x:%x:%x:%x", &vals[0], &vals[1], &vals[2], &vals[3], &vals[4], &vals[5]) == 6) {
            for (int i = 0; i < 6; ++i) mymac[i] = (uint8_t)vals[i];
            if (memcmp(mymac, KNOWN_MAC_A, 6) == 0) {
                memcpy(espnow_peer_mac, KNOWN_MAC_B, 6);
            } else if (memcmp(mymac, KNOWN_MAC_B, 6) == 0) {
                memcpy(espnow_peer_mac, KNOWN_MAC_A, 6);
            } else {
                Serial.printf("Unknown board MAC %s; peer must be set manually in code\n", macstr.c_str());
            }
        } else {
            Serial.println("Failed to parse own MAC address");
        }
    }
    if (memcmp(espnow_peer_mac, (uint8_t[]){0,0,0,0,0,0}, 6) != 0) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, espnow_peer_mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("Failed to add ESP-NOW peer");
        } else {
            Serial.println("ESP-NOW peer added");
        }
    } else {
        Serial.println("ESP-NOW peer MAC is all zeros; not adding peer (set espnow_peer_mac in code)");
    }
}
#endif

//Idk what im doing by tuping here, idek if Im going to make this work properly. I'm so far behind on this I had to come in today to work on this, and I shouldn't, cause I should be working on other stuff.
// --- Configuration ---
// WiFi credentials and router IP (ESP32-only)
#if defined(ESP32)
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* router_ip = "192.168.1.100"; // Change to your router's IP
const uint16_t router_port __attribute__((unused)) = 5005;
#endif

// MIDI UART config (defined in src/teachtiles.h)

// On ESP32/Arduino we use the platform Serial2 and BluetoothSerial; host stubs are above
#if defined(ESP32)
#include "HardwareSerial.h"
// Serial2 instance is provided by the ESP32 Arduino core; don't redefine it here.
#if USE_BT
BluetoothSerial SerialBT;
#endif
#endif

// Fallback define for SERIAL_8N1 on host builds
#if !defined(SERIAL_8N1)
#define SERIAL_8N1 0
#endif

// MIDI message parsing state (type defined in header)
MidiParseState midiState = WAIT_STATUS;
uint8_t midiStatus = 0;
uint8_t midiNote = 0;
// Array to store note-on timestamps for each MIDI note (0-127)
uint32_t noteOnTime[128] = {0};
// Track signal presence and current playing note for serial output
uint32_t lastReceivedMillis = 0;
bool signalPresent = false;
int currentPlayingNote = -1; // -1 when no note
uint8_t currentVelocityVal = 0;
uint32_t currentNoteStart = 0;
// Periodic status print interval and raw dump intervals are in src/teachtiles.h
uint32_t lastStatusPrintMillis = 0;
uint32_t lastRawDumpMillis = 0;
// temporary buffer for raw bytes read in each loop cycle
uint8_t rawBuf[64];
size_t rawBufLen = 0;
// GPIO-level diagnostics
int lastRxPinState = -1;
// Pin check timing
uint32_t lastPinCheckMillis = 0;

// Helper: convert MIDI note number to note name (e.g., 60 -> C4)
const char* midiNoteToName(uint8_t note) {
    static char buf[8];
    static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    int n = note % 12;
    int octave = (note / 12) - 1; // MIDI octave numbering
    snprintf(buf, sizeof(buf), "%s%d", names[n], octave);
    return buf;
}
void sendNoteData(uint8_t note, uint32_t duration) {
    // Package: [note, duration (ms, 4 bytes)]
    uint8_t packet[5];
    packet[0] = note;
    // pack big-endian (MSB first) — tests expect p[1]<<24 ... p[4]
    packet[1] = (duration >> 24) & 0xFF;
    packet[2] = (duration >> 16) & 0xFF;
    packet[3] = (duration >> 8) & 0xFF;
    packet[4] = duration & 0xFF;
    // Send via selected transport
    // If Bluetooth is enabled and a client is connected, send via SPP
    if (transport == TM_BT) {
#if USE_BT
    if (SerialBT.hasClient()) {
        SerialBT.write(packet, 5);
    } else {
        Serial.printf("(bt) no client connected; would send note %d dur %lu\n", packet[0], (unsigned long)duration);
    }
#else
    Serial.printf("(bt disabled) would send packet for note %d dur %lu\n", packet[0], (unsigned long)duration);
#endif
    } else if (transport == TM_ESPNOW) {
#if defined(ESP32)
        if (memcmp(espnow_peer_mac, (uint8_t[]){0,0,0,0,0,0}, 6) != 0) {
            esp_err_t r = esp_now_send(espnow_peer_mac, packet, 5);
            if (r != ESP_OK) Serial.printf("ESP-NOW send er ror: %d\n", r);
        } else {
            Serial.println("ESP-NOW peer not configured; cannot send");
        }
#else
        // Host: fall back to SerialBT
        SerialBT.write(packet, 5);
#endif
    } else if (transport == TM_UDP) {
#if defined(ESP32)
        _udp.beginPacket(router_ip, router_port);
        _udp.write(packet, 5);
        _udp.endPacket();
#else
        // Host fallback: SerialBT captures
        SerialBT.write(packet, 5);
#endif
    }
}
void setup() {
    Serial.begin(115200); // Debug output
    // Initialize MIDI RX and Bluetooth for both host tests and ESP32
    // Use explicit SERIAL_8N1 for MIDI UART config. If your MIDI interface inverts the signal
    // you may need to use SERIAL_8N1 (default) or invert wiring/driver. If you used a raw 3
    // earlier, that may not be portable across cores; use the Arduino constant instead.
    Serial2.begin(MIDI_BAUDRATE, SERIAL_8N1, MIDI_RX_PIN, -1); // host stub ignores args
#if USE_BT
    if (!SerialBT.begin("TeachTile")) {
        Serial.println("Failed to start Bluetooth");
    } else {
        Serial.println("Bluetooth SPP started (TeachTile)");
    }
#else
    Serial.println("Bluetooth disabled at compile time (USE_BT=0)");
#endif
    // Print typical serial monitor startup info for ESP32
#if defined(ESP32)
    Serial.println();
    Serial.println("--- ESP32 Serial Monitor ---");
    Serial.printf("SDK: %s\n", ESP.getSdkVersion());
    Serial.printf("Chip: %s (rev %u)\n", ESP.getChipModel(), ESP.getChipRevision());
    uint64_t mac = ESP.getEfuseMac();
    Serial.printf("MAC: %012llX\n", mac);
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    // Configure RX pin input pullup so we can sense electrical transitions for debugging
    pinMode(MIDI_RX_PIN, INPUT_PULLUP);
    lastRxPinState = digitalRead(MIDI_RX_PIN);
    // Initialize transport-specific networking
    // Initialize ESP-NOW so this board can receive ESPNOW packets even if we prefer BT.
    // That lets us use ESP-NOW as an additional wireless transport alongside Bluetooth SPP.
    Serial.println("Init: attempting to start ESP-NOW (if supported)");
    initEspNow();

    if (transport == TM_UDP) {
        Serial.println("Transport: UDP -> connecting to WiFi");
        WiFi.begin(ssid, password);
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - start) < 10000) {
            delay(200);
            Serial.print('.');
        }
    Serial.println("");
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
            _udp.begin(router_port);
        } else {
            Serial.println("WiFi connect failed (UDP transport may not work)");
        }
    }
#else
    Serial.println("Ready to receive MIDI...");
#endif
    // Initialize Monalith visualizer
    if (!Monalith::init()) {
        Serial.println("Monalith init failed");
    }
    // Short visual test: flash full red -> off -> full red (ESP32 only) so we can
    // confirm the panel is connected and receiving data. This is temporary.
#if defined(ESP32)
    extern const uint16_t example_bitmap[]; // defined in example_bitmap.c
    Monalith::showStaticBitmap(example_bitmap);
    Monalith::setDisplayState(Monalith::DisplayState::StaticBitmap);
#else
    extern const uint16_t example_bitmap[]; // defined in example_bitmap.c
    Monalith::showStaticBitmap(example_bitmap);
    Monalith::setDisplayState(Monalith::DisplayState::StaticBitmap);
#endif
    // Auto-clear the static bitmap after 60 seconds so the overlay is visible
    // for a measured period and then the display returns to normal behavior.
#if defined(ESP32)
    // Use a simple lambda+task to delay and clear without blocking setup().
    auto clear_task = [](void*) {
        const uint32_t delay_ms = 60000;
        vTaskDelay(delay_ms / portTICK_PERIOD_MS);
        Monalith::clearStaticBitmap();
        // delete this task
        vTaskDelete(nullptr);
    };
    // create a low-priority task to perform the delayed clear
    xTaskCreatePinnedToCore(clear_task, "clear_bitmap", 2048, nullptr, 1, nullptr, 1);
#endif
}

void loop() {
    // Read incoming bytes into rawBuf for optional hex dump
    rawBufLen = 0;
    while (Serial2.available()) {
        uint8_t byte = Serial2.read();
        if (rawBufLen < sizeof(rawBuf)) rawBuf[rawBufLen++] = byte;

        // MIDI message parsing (only note-on and note-off)
        if (byte & 0x80) { // Status byte
            midiStatus = byte;
            midiState = WAIT_NOTE;
        } else {
            switch (midiState) {
                case WAIT_NOTE:
                    midiNote = byte;
                    midiState = WAIT_VELOCITY;
                    break;
                case WAIT_VELOCITY:
                    // Update last received time (signal present)
                    lastReceivedMillis = millis();
                    signalPresent = true;

                    if ((midiStatus & 0xF0) == 0x90 && byte > 0) {
                        // Note ON
                        uint32_t t = millis();
                        if (t == 0) t = 1; // avoid 0 sentinel
                        noteOnTime[midiNote] = t;
                        currentPlayingNote = midiNote;
                        currentVelocityVal = byte;
                        currentNoteStart = noteOnTime[midiNote];
                        Serial.printf("Signal: true | Note: %s (%d) | Velocity: %d | State: ON\n", midiNoteToName(midiNote), midiNote, currentVelocityVal);
                    } else if (((midiStatus & 0xF0) == 0x80) || ((midiStatus & 0xF0) == 0x90 && byte == 0)) {
                        // Note OFF
                        uint32_t now = millis();
                        uint32_t duration = 0;
                        if (noteOnTime[midiNote] != 0) {
                            duration = now - noteOnTime[midiNote];
                            if (duration == 0) duration = 1; // ensure non-zero for very short holds
                        }
                        Serial.printf("Signal: true | Note: %s (%d) | Velocity: %d | State: OFF | Duration: %lu ms\n", midiNoteToName(midiNote), midiNote, (unsigned)byte, duration);
                        // Only send if we had a prior note-on recorded
                        if (noteOnTime[midiNote] != 0) {
                            sendNoteData(midiNote, duration);
                        } else {
                            Serial.printf("(info) Ignored OFF for note %d with no prior ON\n", midiNote);
                        }
                        noteOnTime[midiNote] = 0;
                        // If the note that turned off was the currently tracked one, clear it
                        if (currentPlayingNote == midiNote) {
                            currentPlayingNote = -1;
                            currentVelocityVal = 0;
                            currentNoteStart = 0;
                        }
                    }
                    midiState = WAIT_NOTE;
                    break;
                default:
                    midiState = WAIT_STATUS;
                    break;
            }
        }
    }
    // Dump raw MIDI bytes occasionally for debug if any were read
    uint32_t now = millis();
    if (rawBufLen > 0 && (now - lastRawDumpMillis) >= RAW_MIDI_DUMP_MS) {
        Serial.print("Raw MIDI bytes: ");
        for (size_t i = 0; i < rawBufLen; ++i) {
            Serial.printf("%02X ", rawBuf[i]);
        }
    Serial.println("");
        lastRawDumpMillis = now;
    }
    // No periodic status printing: updates are printed only when incoming MIDI events are parsed
    // Add a lightweight periodic status print so we always know whether a note is playing.
    // If we haven't seen any MIDI activity for a short while, clear signalPresent/currentPlayingNote
    if (signalPresent && (now - lastReceivedMillis) > 2000) {
        signalPresent = false;
        currentPlayingNote = -1;
        currentVelocityVal = 0;
    }
    if ((now - lastStatusPrintMillis) >= STATUS_PRINT_INTERVAL_MS) {
        lastStatusPrintMillis = now;
        if (currentPlayingNote != -1) {
            uint32_t playingFor = now - currentNoteStart;
            Serial.printf("Playing: true | Note: %s (%d) | Velocity: %d | Playing for: %lums\n",
                          midiNoteToName((uint8_t)currentPlayingNote), currentPlayingNote, currentVelocityVal, (unsigned long)playingFor);
        } else {
            Serial.println("Playing: false");
        }
    }

#if defined(ESP32)
    // Sample the raw RX pin occasionally and print transitions for electrical diagnostics
    if ((now - lastPinCheckMillis) >= PIN_CHECK_INTERVAL_MS) {
        lastPinCheckMillis = now;
        int s = digitalRead(MIDI_RX_PIN);
        if (s != lastRxPinState) {
            lastRxPinState = s;
            Serial.printf("RX pin state changed: %s\n", s ? "HIGH" : "LOW");
        }
    }
#endif

#if defined(ESP32)
#if USE_BT
    // Check for incoming Bluetooth SPP packets (5-byte note packets)
    if (SerialBT.hasClient()) {
        while (SerialBT.available() >= 5) {
            uint8_t buf[5];
            size_t r = SerialBT.readBytes(buf, 5);
            if (r == 5) {
                uint8_t note = buf[0];
                uint32_t duration = ((uint32_t)buf[1]<<24) | ((uint32_t)buf[2]<<16) | ((uint32_t)buf[3]<<8) | (uint32_t)buf[4];
                Serial.printf("(BT RX) Note: %s (%d) | Duration: %lu ms\n", midiNoteToName(note), note, duration);
                Monalith::showNote(note, duration);
            }
        }
    }
#endif
#if defined(ESP32)
    // UDP receive: listen for 5-byte packets from the bridge/host
    if (transport == TM_UDP) {
        int packetSize = _udp.parsePacket();
        if (packetSize >= 5) {
            uint8_t buf[5];
            int len = _udp.read(buf, 5);
            if (len == 5) {
                uint8_t note = buf[0];
                uint32_t duration = ((uint32_t)buf[1]<<24) | ((uint32_t)buf[2]<<16) | ((uint32_t)buf[3]<<8) | (uint32_t)buf[4];
                Serial.printf("(UDP RX) Note: %s (%d) | Duration: %lu ms\n", midiNoteToName(note), note, duration);
                // Forward to visualizer
                Monalith::showNote(note, duration);
            } else {
                // drain the packet if it's larger/shorter to avoid re-reading
                uint8_t drainBuf[64];
                while (_udp.available()) _udp.read(drainBuf, sizeof(drainBuf));
            }
        }
    }
#endif
#endif
    // Advance visualizer animations
    Monalith::tick();
}