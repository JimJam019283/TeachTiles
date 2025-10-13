// main.cpp
// ESP32 MIDI Note Reader and Router
// This program reads MIDI signals from a digital piano via UART,
// extracts note-on and note-off events (note number and duration),
// and packages the data to send to a router (e.g., via UDP).
// Designed for ESP32 WROOM module using Arduino framework.

// Include Arduino headers only when building for Arduino/ESP32
#if defined(ARDUINO) || defined(ESP32)
#include <Arduino.h>
#include "BluetoothSerial.h"
#include <stdint.h>
#else
// Host (non-Arduino) build: provide minimal stubs so the file can be compiled/run locally
#include <cstdint>
#include <chrono>
#include <cstdio>
#include <cstdarg>

struct DummySerial {
    void begin(int) {}
    void println(const char* s) { std::puts(s); }
    void print(const char* s) { std::fputs(s, stdout); }
    void printf(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); }
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


// --- Configuration ---
// WiFi credentials and router IP
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* router_ip = "192.168.1.100"; // Change to your router's IP
const uint16_t router_port = 5005;

// MIDI UART config
#define MIDI_RX_PIN 16 // Connect MIDI OUT from piano to this pin
#define MIDI_BAUDRATE 31250

// On ESP32/Arduino we use the platform Serial2 and BluetoothSerial; host stubs are above
#if defined(ESP32)
#include "HardwareSerial.h"
// Serial2 instance is provided by the ESP32 Arduino core; don't redefine it here.
BluetoothSerial SerialBT;
#endif

// MIDI message parsing state
enum MidiParseState {
    WAIT_STATUS,
    WAIT_NOTE,
    WAIT_VELOCITY
};

MidiParseState midiState = WAIT_STATUS;
uint8_t midiStatus = 0;
uint8_t midiNote = 0;
// Array to store note-on timestamps for each MIDI note (0-127)
uint32_t noteOnTime[128] = {0};
void sendNoteData(uint8_t note, uint32_t duration) {
    // Package: [note, duration (ms, 4 bytes)]
    uint8_t packet[5];
    packet[0] = note;
    packet[1] = (duration >> 24) & 0xFF;
    packet[2] = (duration >> 16) & 0xFF;
    packet[3] = (duration >> 8) & 0xFF;
    packet[4] = duration & 0xFF;
    // Send the 5-byte packet over Bluetooth SPP (host stub captures this)
    SerialBT.write(packet, 5);
}
void setup() {
    Serial.begin(115200); // Debug output
    // Initialize MIDI RX and Bluetooth for both host tests and ESP32
    Serial2.begin(MIDI_BAUDRATE, 3, MIDI_RX_PIN, -1); // host stub ignores args
    if (!SerialBT.begin("TeachTile")) {
        Serial.println("Failed to start Bluetooth");
    } else {
        Serial.println("Bluetooth SPP started (TeachTile)");
    }
    Serial.println("Ready to receive MIDI...");
}

void loop() {
    while (Serial2.available()) {
        uint8_t byte = Serial2.read();

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
                    if ((midiStatus & 0xF0) == 0x90 && byte > 0) {
                        // Note ON
                        noteOnTime[midiNote] = millis();
                        Serial.printf("Note ON: %d\n", midiNote);
                    } else if (((midiStatus & 0xF0) == 0x80) || ((midiStatus & 0xF0) == 0x90 && byte == 0)) {
                        // Note OFF
                        uint32_t duration = millis() - noteOnTime[midiNote];
                        Serial.printf("Note OFF: %d, Duration: %lu ms\n", midiNote, duration);
                        sendNoteData(midiNote, duration);
                        noteOnTime[midiNote] = 0;
                    }
                    midiState = WAIT_NOTE;
                    break;
                default:
                    midiState = WAIT_STATUS;
                    break;
            }
        }
    }
}