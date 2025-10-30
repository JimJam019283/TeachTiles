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

// Show a persistent 64x64 bitmap on the matrix. The bitmap is expected to be
// an array of WIDTH*HEIGHT uint16_t values in 16-bit RGB565 format. The bitmap
// is copied internally; it will remain displayed until `clearStaticBitmap()`
// is called. Call `tick()` regularly from the main loop to keep the panel
// refreshed.
void showStaticBitmap(const uint16_t* bitmap);

// Clear any persistent static bitmap and return to normal animated behavior.
void clearStaticBitmap();

// Simple display state machine. Used by higher-level code to request what the
// display should show.
enum class DisplayState {
	Normal,       // animated notes and normal behavior
	StaticBitmap, // show the persistent 64x64 bitmap
};

// Set the desired display state. When entering `StaticBitmap`, the current
// static bitmap (if any) will be shown. `getDisplayState()` returns the
// current state.
void setDisplayState(DisplayState s);
DisplayState getDisplayState();

} // namespace Monalith
