// host_main.cpp
// Host-friendly version of the Arduino sketch for local testing.
#ifndef ARDUINO
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <iostream>
#include <chrono>
#include <thread>
#include <array>

using namespace std::chrono;

// --- Minimal Arduino-like API stubs ---
struct SerialPort {
    void begin(int) {}
    void println(const char* s) { std::cout << s << std::endl; }
    void println(const std::string &s) { std::cout << s << std::endl; }
    void print(const char* s) { std::cout << s; }
    void printf(const char* fmt, ...) {
        va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
    }
};
//Serial Stuff
SerialPort Serial;
// Minimal Serial2 stub: no incoming data by default
struct Serial2_t {
    void begin(int, int, int, int) {}
    int available() { return 0; }
    uint8_t read() { return 0; }
} Serial2;

uint32_t millis() {
    return (uint32_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void delay(unsigned long ms) { std::this_thread::sleep_for(milliseconds(ms)); }

// WiFi/UDP stubs
struct WiFiClass { void begin(const char*, const char*) {} int status() { return 3; } std::string localIP() { return "127.0.0.1"; } } WiFi;
struct WiFiUDP {
    void begin(unsigned) {}
    void beginPacket(const char*, uint16_t) {}
    void write(const uint8_t*, size_t n) { std::cout << "UDP packet (" << n << " bytes)\n"; }
    void endPacket() {}
} udp;

// --- Application logic (adapted from original sketch) ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* router_ip = "127.0.0.1";
const uint16_t router_port = 5005;

#define MIDI_RX_PIN 16
#define MIDI_BAUDRATE 31250

enum MidiParseState { WAIT_STATUS, WAIT_NOTE, WAIT_VELOCITY };
MidiParseState midiState = WAIT_STATUS;
uint8_t midiStatus = 0;
uint8_t midiNote = 0;
uint32_t noteOnTime[128] = {0};

void setupWiFi() {
    Serial.println("(host) Connecting to WiFi...");
    WiFi.begin(ssid, password);
    // pretend connected
    Serial.println("(host) WiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void sendNoteData(uint8_t note, uint32_t duration) {
    uint8_t packet[5];
    packet[0] = note;
    packet[1] = (duration >> 24) & 0xFF;
    packet[2] = (duration >> 16) & 0xFF;
    packet[3] = (duration >> 8) & 0xFF;
    packet[4] = duration & 0xFF;
    udp.beginPacket(router_ip, router_port);
    udp.write(packet, 5);
    udp.endPacket();
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(MIDI_BAUDRATE, 0, MIDI_RX_PIN, -1);
    setupWiFi();
    udp.begin(12345);
    Serial.println("(host) Ready to receive MIDI...");
}

void loop() {
    // No real Serial2 data on host; this is a placeholder.
    while (Serial2.available()) {
        uint8_t byte = Serial2.read();
        if (byte & 0x80) {
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
                        noteOnTime[midiNote] = millis();
                        Serial.printf("Note ON: %d\n", midiNote);
                    } else if (((midiStatus & 0xF0) == 0x80) || ((midiStatus & 0xF0) == 0x90 && byte == 0)) {
                        uint32_t duration = millis() - noteOnTime[midiNote];
                        Serial.printf("Note OFF: %d, Duration: %u ms\n", midiNote, duration);
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

int main(int argc, char** argv) {
    setup();
    while (true) {
        loop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}
#endif
