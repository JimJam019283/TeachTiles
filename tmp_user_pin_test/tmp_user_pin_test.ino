#include <Arduino.h>
#include <PxMatrix.h>

const uint16_t WIDTH = 64;
const uint16_t HEIGHT = 64;

// User-provided mapping (from your message)
#define P_R1_PIN 25
#define P_G1_PIN 26
#define P_B1_PIN 27
#define P_R2_PIN 14
#define P_G2_PIN 12
#define P_B2_PIN 13
#define P_E_PIN 32
#define P_A_PIN 23
#define P_B_PIN 22
#define P_C_PIN 19
#define P_D_PIN 5
#define P_CLK_PIN 18
#define P_LAT_PIN 4
#define P_OE_PIN 15

PxMATRIX display(WIDTH, HEIGHT, P_LAT_PIN, P_OE_PIN, P_A_PIN, P_B_PIN, P_C_PIN, P_D_PIN, P_E_PIN);

void fillWhite() {
  uint16_t color565 = 0xFFFF; // RGB565 white
  for (uint16_t y = 0; y < HEIGHT; ++y) {
    for (uint16_t x = 0; x < WIDTH; ++x) {
      display.drawPixel(x, y, color565);
    }
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("tmp_user_pin_test: start");
  display.begin(8); // multiplex 8 (common for 64x64)
  display.setBrightness(255);
  fillWhite();
  Serial.println("tmp_user_pin_test: filled white");
}

void loop() {
  delay(1000);
}
