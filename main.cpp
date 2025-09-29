// main.cpp
// ESP32 MIDI Note Reader and Router
// This program reads MIDI signals from a digital piano via UART,
// extracts note-on and note-off events (note number and duration),
// and packages the data to send to a router (e.g., via UDP).
// Designed for ESP32 WROOM module using Arduino framework.

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// WiFi credentials and router IP
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* router_ip = "192.168.1.100"; // Change to your router's IP
const uint16_t router_port = 5005;

// MIDI UART config
#define MIDI_RX_PIN 16 // Connect MIDI OUT from piano to this pin
#define MIDI_BAUDRATE 31250

// UDP object
WiFiUDP udp;

// MIDI message parsing state
enum MidiParseState {
    WAIT_STATUS,
    WAIT_NOTE,
    WAIT_VELOCITY
};

MidiParseState midiState = WAIT_STATUS;
uint8_t midiStatus = 0;
uint8_t midiNote = 0;
uint32_t noteOnTime[128] = {0}; // Store note-on times for each note

void setupWiFi() {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void sendNoteData(uint8_t note, uint32_t duration) {
    // Package: [note, duration (ms, 4 bytes)]
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
    Serial.begin(115200); // Debug output
    Serial2.begin(MIDI_BAUDRATE, SERIAL_8N1, MIDI_RX_PIN, -1); // MIDI UART
    setupWiFi();
    udp.begin(12345); // Local UDP port (arbitrary)
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