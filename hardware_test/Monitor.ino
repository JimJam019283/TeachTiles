// Monitor.ino
// Simple ESP32 MIDI monitor: prints incoming MIDI bytes and interprets note on/off
#include <Arduino.h>

#define MIDI_RX_PIN 16
#define MIDI_BAUDRATE 31250

enum MidiParseState { WAIT_STATUS, WAIT_NOTE, WAIT_VELOCITY };
MidiParseState midiState = WAIT_STATUS;
uint8_t midiStatus = 0;
uint8_t midiNote = 0;
uint32_t noteOnTime[128] = {0};

void setup() {
  Serial.begin(115200);
  Serial2.begin(MIDI_BAUDRATE, SERIAL_8N1, MIDI_RX_PIN, -1);
  Serial.println("MIDI Monitor started. Listening on Serial2...");
}

void loop() {
  while (Serial2.available()) {
    uint8_t b = Serial2.read();
    // print raw byte
    Serial.printf("BYTE: 0x%02X\n", b);
    if (b & 0x80) {
      midiStatus = b;
      midiState = WAIT_NOTE;
      Serial.printf("STATUS: 0x%02X\n", midiStatus);
    } else {
      switch (midiState) {
        case WAIT_NOTE:
          midiNote = b;
          midiState = WAIT_VELOCITY;
          break;
        case WAIT_VELOCITY: {
          uint8_t vel = b;
          if ((midiStatus & 0xF0) == 0x90 && vel > 0) {
            noteOnTime[midiNote] = millis();
            Serial.printf("NOTE ON: note=%d vel=%d t=%lu\n", midiNote, vel, noteOnTime[midiNote]);
          } else if (((midiStatus & 0xF0) == 0x80) || ((midiStatus & 0xF0) == 0x90 && vel == 0)) {
            uint32_t duration = millis() - noteOnTime[midiNote];
            Serial.printf("NOTE OFF: note=%d vel=%d duration=%lu ms\n", midiNote, vel, duration);
            noteOnTime[midiNote] = 0;
          }
          midiState = WAIT_NOTE;
        } break;
        default:
          midiState = WAIT_STATUS;
          break;
      }
    }
  }
}
