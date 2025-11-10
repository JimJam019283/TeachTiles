#include "../monalith/monalith.h"

static const uint16_t WHITE = 0xFFFF;
static uint16_t whiteBitmap[64*64];

void setup() {
  for (int i = 0; i < 64*64; ++i) whiteBitmap[i] = WHITE;
  Serial.begin(115200);
  delay(50);
  if (Monalith::init()) Serial.println("Monalith init ok");
  Monalith::showStaticBitmap(whiteBitmap);
}

void loop() { delay(1000); }
