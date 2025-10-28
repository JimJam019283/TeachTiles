#include <Arduino.h>
// Ensure ESP32 GPIO definitions are available for PxMatrix macros
#include <driver/gpio.h>
#include <soc/gpio_struct.h>
#include <PxMatrix.h>

// We'll try several common pin mappings for Waveshare 64x64 panels on ESP32.
// Each mapping is used for a few seconds so you can visually identify which
// wiring matches your panel. If one mapping lights the panel red, note which
// mapping number printed on serial and we'll use that for further tests.

const uint16_t WIDTH = 64;
const uint16_t HEIGHT = 64;

struct Mapping { int P_LAT; int P_OE; int P_A; int P_B; int P_C; int P_D; int P_E; };

// Candidate mappings (common variants). Adjust or add more if needed.
Mapping mappings[] = {
  // Mapping set 0: earlier used in many examples
  {22, 21, 19, 18, 5, 15, -1},
  // Mapping set 1: Waveshare diagram variant
  {22, 21, 16, 17, 2, 4, 15},
  // Mapping set 2: alternate (ESP32-HUB75 examples)
  {27, 26, 16, 17, 21, 22, 15}
};

void fillWithRed(PxMATRIX &display) {
  uint8_t r = 255, g = 0, b = 0;
  uint16_t color565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  for (uint16_t y = 0; y < HEIGHT; ++y) {
    for (uint16_t x = 0; x < WIDTH; ++x) {
      display.drawPixel(x, y, color565);
    }
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  Serial.println("hw_test_red: mapping sweep starting");

  for (int i = 0; i < (int)(sizeof(mappings)/sizeof(mappings[0])); ++i) {
    auto m = mappings[i];
    Serial.printf("Trying mapping %d: LAT=%d OE=%d A=%d B=%d C=%d D=%d E=%d\n", i, m.P_LAT, m.P_OE, m.P_A, m.P_B, m.P_C, m.P_D, m.P_E);
    // Create PxMATRIX instance with or without E pin depending on mapping
    PxMATRIX display(WIDTH, HEIGHT, m.P_LAT, m.P_OE, m.P_A, m.P_B, m.P_C, m.P_D);
    display.begin();
    fillWithRed(display);
    delay(3000); // show for 3 seconds
    // clear framebuffer by drawing black
    uint8_t r = 0, g = 0, b = 0;
    uint16_t black = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    for (uint16_t y = 0; y < HEIGHT; ++y) for (uint16_t x = 0; x < WIDTH; ++x) display.drawPixel(x, y, black);
    display.display();
    delay(200);
  }
  Serial.println("hw_test_red: mapping sweep complete. If none lit the panel, try alternate wiring or report back.");
}

void loop() {
  delay(1000);
}
