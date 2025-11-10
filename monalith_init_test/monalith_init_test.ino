#include "../monalith/monalith.h"

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("monalith_init_test: start");
  bool ok = Monalith::init();
  Serial.print("monalith_init_test: init returned ");
  Serial.println(ok ? "true" : "false");
}

void loop() { delay(1000); }
