// main.cpp
// ESP32 MIDI Note Reader and Router
// This program reads MIDI signals from a digital piano via UART,
// extracts note-on and note-off events (note number and duration),
// and packages the data to send to a router (e.g., via UDP).
// Designed for ESP32 WROOM module using Arduino framework.

#include <Arduino.h>
#include "BluetoothSerial.h"

// WiFi credentials and router IP
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* router_ip = "192.168.1.100"; // Change to your router's IP
const uint16_t router_port = 5005;

// MIDI UART config
#define MIDI_RX_PIN 16 // Connect MIDI OUT from piano to this pin
#define MIDI_BAUDRATE 31250

// Bluetooth serial (ESP32 Classic SPP)
BluetoothSerial SerialBT;

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

void sendNoteData(uint8_t note, uint32_t duration) {
    // Package: [note, duration (ms, 4 bytes)]
    uint8_t packet[5];
    packet[0] = note;
    packet[1] = (duration >> 24) & 0xFF;
    packet[2] = (duration >> 16) & 0xFF;
    packet[3] = (duration >> 8) & 0xFF;
    packet[4] = duration & 0xFF;
    // Send the 5-byte packet over Bluetooth SPP
    SerialBT.write(packet, 5);
}

void setup() {
    Serial.begin(115200); // Debug output
    Serial2.begin(MIDI_BAUDRATE, SERIAL_8N1, MIDI_RX_PIN, -1); // MIDI UART
    // Start Bluetooth SPP and advertise device name
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