#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
}

void loop() {
  digitalWrite(2, HIGH);
  Serial.println("blink: HIGH");
  delay(500);
  digitalWrite(2, LOW);
  Serial.println("blink: LOW");
  delay(500);
}
