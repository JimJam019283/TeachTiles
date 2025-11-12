void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("--- MINITEST BOOT ---");
  Serial.println("Serial-only test: printing alive every second");
}

void loop() {
  Serial.println("alive");
  delay(1000);
}
