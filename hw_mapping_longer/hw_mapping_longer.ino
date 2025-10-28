#include <Arduino.h>
#include <driver/gpio.h>
#include <soc/gpio_struct.h>
#include <PxMatrix.h>

const uint16_t WIDTH = 64;
const uint16_t HEIGHT = 64;

struct Mapping { const char* name; int lat; int oe; int a; int b; int c; int d; int e; };

// More exhaustive candidate mappings with longer display durations
Mapping mappings[] = {
  {"waveshare1", 22, 21, 16, 17, 2, 4, 15},
  {"waveshare2", 22, 21, 19, 18, 5, 15, -1},
  {"esp32-alt1", 27, 26, 16, 17, 21, 22, 15},
  {"common-1", 13, 12, 27, 14, 26, 25, -1},
  {"i2s-1", 22, 21, 25, 26, 27, 14, -1},
  {"alt-2", 14, 12, 23, 18, 5, 19, -1},
  // try a couple more permutations
  {"perm-1", 22, 21, 23, 18, 5, 19, -1},
  {"perm-2", 22, 21, 5, 18, 23, 19, -1}
};

void tryMapping(const Mapping &m, int ms) {
  Serial.printf("Trying %s LAT=%d OE=%d A=%d B=%d C=%d D=%d E=%d for %d ms\n", m.name, m.lat, m.oe, m.a, m.b, m.c, m.d, m.e, ms);
  PxMATRIX display(WIDTH, HEIGHT, m.lat, m.oe, m.a, m.b, m.c, m.d);
  display.begin();
  // fill matrix with red for ms duration
  uint8_t r = 255, g = 0, b = 0;
  uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  for (uint16_t y = 0; y < HEIGHT; ++y) for (uint16_t x = 0; x < WIDTH; ++x) display.drawPixel(x, y, color);
  display.display();
  delay(ms);
  // clear
  uint16_t black = 0;
  for (uint16_t y = 0; y < HEIGHT; ++y) for (uint16_t x = 0; x < WIDTH; ++x) display.drawPixel(x, y, black);
  display.display();
  delay(200);
}

void setup() {
  Serial.begin(115200);
  Serial.println("hw_mapping_longer: start");
  for (int i = 0; i < (int)(sizeof(mappings)/sizeof(mappings[0])); ++i) {
    tryMapping(mappings[i], 7000); // 7 seconds per mapping
  }
  Serial.println("hw_mapping_longer: done. If none lit, check power or ribbon orientation.");
}

void loop() { delay(1000); }
