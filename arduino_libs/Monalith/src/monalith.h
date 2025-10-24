#pragma once

#include <stdint.h>

namespace Monalith {

bool init();
void showNote(uint8_t note, uint32_t duration_ms, uint8_t velocity = 100);
void tick();

} // namespace Monalith
