#include <Arduino.h>
#include <driver/gpio.h>
#include <soc/gpio_struct.h>
#include <PxMatrix.h>

// Use the Waveshare mapping we tried earlier
#define P_LAT 22
#define P_OE 21
#define P_A 16
#define P_B 17
#define P_C 2
#define P_D 4

const uint16_t WIDTH = 64;
const uint16_t HEIGHT = 64;

PxMATRIX display(WIDTH, HEIGHT, P_LAT, P_OE, P_A, P_B, P_C, P_D);

void setup() {
  Serial.begin(115200);
  Serial.println("hw_cycle_rgb: starting");
  display.begin();
  // clear
  uint16_t black = 0;
  for (uint16_t y=0;y<HEIGHT;++y) for (uint16_t x=0;x<WIDTH;++x) display.drawPixel(x,y,black);
  display.display();
}

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void showColor(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t col = rgb565(r,g,b);
  uint16_t cx = WIDTH/2, cy = HEIGHT/2;
  display.drawPixel(cx, cy, col);
  display.display();
}

void loop() {
  Serial.println("hw_cycle_rgb: RED");
  showColor(255,0,0);
  delay(800);
  Serial.println("hw_cycle_rgb: GREEN");
  showColor(0,255,0);
  delay(800);
  Serial.println("hw_cycle_rgb: BLUE");
  showColor(0,0,255);
  delay(800);
}
