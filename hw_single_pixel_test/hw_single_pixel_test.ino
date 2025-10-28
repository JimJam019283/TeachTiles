#include <Arduino.h>
#include <driver/gpio.h>
#include <soc/gpio_struct.h>
#include <PxMatrix.h>

// Waveshare mapping we used earlier
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
  Serial.println("hw_single_pixel_test: starting");
  display.begin();
  // ensure cleared
  uint16_t black = 0;
  for (uint16_t y=0;y<HEIGHT;++y) for (uint16_t x=0;x<WIDTH;++x) display.drawPixel(x,y,black);
  display.display();
}

void loop() {
  // toggle center pixel on/off every 1s
  uint16_t cx = WIDTH/2;
  uint16_t cy = HEIGHT/2;
  uint8_t r = 255, g = 0, b = 0;
  uint16_t red = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  uint16_t black = 0;
  Serial.println("hw_single_pixel_test: set red");
  display.drawPixel(cx, cy, red);
  display.display();
  delay(1000);
  Serial.println("hw_single_pixel_test: clear");
  display.drawPixel(cx, cy, black);
  display.display();
  delay(1000);
}
