#include <Arduino.h>
#include <driver/gpio.h>
#include <soc/gpio_struct.h>
#include <PxMatrix.h>

const uint16_t WIDTH = 64;
const uint16_t HEIGHT = 64;

struct Mapping { const char* name; int lat; int oe; int a; int b; int c; int d; int e; };

// Expanded set of candidate mappings used by various ESP32 HUB75 examples
Mapping mappings[] = {
  {"set0", 22, 21, 19, 18, 5, 15, -1},
  {"set1-waveshare", 22, 21, 16, 17, 2, 4, 15},
  {"set2-alt", 27, 26, 16, 17, 21, 22, 15},
  {"set3-common", 13, 12, 27, 14, 26, 25, -1},
  {"set4-i2s", 22, 21, 25, 26, 27, 14, -1},
  {"set5-other", 14, 12, 23, 18, 5, 19, -1}
};

void tryMapping(const Mapping &m) {
  Serial.printf("Trying %s LAT=%d OE=%d A=%d B=%d C=%d D=%d E=%d\n", m.name, m.lat, m.oe, m.a, m.b, m.c, m.d, m.e);
  PxMATRIX display(WIDTH, HEIGHT, m.lat, m.oe, m.a, m.b, m.c, m.d);
  display.begin();
  // draw single pixel in center red
  uint8_t r = 255, g = 0, b = 0;
  uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  uint16_t black = 0;
  uint16_t cx = WIDTH/2, cy = HEIGHT/2;
  display.drawPixel(cx, cy, color);
  display.display();
  delay(3000);
  display.drawPixel(cx, cy, black);
  display.display();
  delay(200);
}

void setup() {
  Serial.begin(115200);
  Serial.println("hw_mapping_expand: start");
  for (int i = 0; i < (int)(sizeof(mappings)/sizeof(mappings[0])); ++i) {
    tryMapping(mappings[i]);
  }
  Serial.println("hw_mapping_expand: done. If none lit, try 'cycle RGB' or verify wiring/power.");
}

void loop() { delay(1000); }
