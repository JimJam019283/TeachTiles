#pragma once

#include <stdint.h>

namespace Monalith {

// Initialize Monalith system and any attached LED matrix hardware.
// Return true on success.
bool init();

// Called when a note with `note` and `duration_ms` is received.
// `velocity` (0-127) influences brightness; default provided for backward compatibility.
// Implementations should map note/duration to LED animations.
void showNote(uint8_t note, uint32_t duration_ms, uint8_t velocity = 100);

// Optional: perform periodic update (call from main loop)
void tick();

// Display a persistent 64x64 bitmap (RGB565). See src/monalith.h for implementation.
void showStaticBitmap(const uint16_t* bitmap);
void clearStaticBitmap();

// Display state machine and control helpers
enum class DisplayState {
	Normal,
	StaticBitmap,
};
void setDisplayState(DisplayState s);
DisplayState getDisplayState();

// Run a short, safe demo for `ms` milliseconds. Non-blocking: returns immediately and
// schedules a short demo sequence; useful for sanity-checking hardware in setup().
void visualizeDemoSafe(uint32_t ms);

// Draw a 'C' glyph with a '#' symbol to its right across the panel for `ms` milliseconds
void drawCSharp(uint32_t ms);

} // namespace Monalith
