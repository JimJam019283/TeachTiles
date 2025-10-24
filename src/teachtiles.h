#pragma once

// teachtiles.h - shared definitions for TeachTiles firmware

#include <stdint.h>

// Transport selection
enum TransportMode { TM_BT = 0, TM_ESPNOW = 1, TM_UDP = 2 };

// MIDI UART settings
constexpr int MIDI_RX_PIN = 16;
constexpr int MIDI_BAUDRATE = 31250;

// Periodic status and debug intervals
constexpr unsigned long STATUS_PRINT_INTERVAL_MS = 200;
constexpr unsigned long RAW_MIDI_DUMP_MS = 500;
constexpr unsigned long PIN_CHECK_INTERVAL_MS = 100;

// MIDI parsing state
enum MidiParseState { WAIT_STATUS, WAIT_NOTE, WAIT_VELOCITY };

// Helper to format note names
const char* midiNoteToName(uint8_t note);

// Firmware entry points
void sendNoteData(uint8_t note, uint32_t duration);

// Transport selection at compile/runtime
extern TransportMode transport;

// ESP-NOW peer MAC helper
#ifdef ESP32
extern uint8_t espnow_peer_mac[6];
#endif

