#include "../monalith/monalith.h"

static const uint16_t WHITE = 0xFFFF;
static uint16_t whiteBitmap[64*64];

void setup() {
  Serial.begin(115200);
  delay(50);
  for (int i = 0; i < 64*64; ++i) whiteBitmap[i] = WHITE;
  Serial.println("white_fast_test: starting");
  bool ok = Monalith::init();
  Serial.print("white_fast_test: Monalith::init() -> ");
  Serial.println(ok ? "true" : "false");
  Monalith::showStaticBitmapFast(whiteBitmap);
  Serial.println("white_fast_test: showStaticBitmapFast called");
}

void loop() { delay(1000); }
