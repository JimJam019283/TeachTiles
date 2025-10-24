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

} // namespace Monalith
