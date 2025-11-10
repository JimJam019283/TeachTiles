#include "../monalith/monalith.h"

void waitSerial() {
  unsigned long start = millis();
  while (millis() - start < 3000) {
    if (Serial) break;
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);
  waitSerial();
  Serial.println("monalith_init_wait: before init");
  delay(50);
  bool ok = Monalith::init();
  Serial.print("monalith_init_wait: init returned ");
  Serial.println(ok ? "true" : "false");
  Serial.println("monalith_init_wait: after init, not calling showStaticBitmap");
}

void loop() { delay(1000); }
