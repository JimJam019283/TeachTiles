// TeachTiles Bitmap Display
// Uses ESP32-HUB75-MatrixPanel-I2S-DMA library

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Include the bitmap data
#include "teachtiles_bitmap.h"

// Panel size
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64

// Pin mapping - verified working
HUB75_I2S_CFG::i2s_pins _pins = {
  25,  // R1
  26,  // G1
  27,  // B1
  14,  // R2
  12,  // G2
  13,  // B2
  23,  // A
  19,  // B
  5,   // C
  17,  // D
  32,  // E
  4,   // LAT
  15,  // OE
  16   // CLK
};

MatrixPanel_I2S_DMA *display = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("=== TeachTiles Bitmap Display ===");
  Serial.println("========================================");
  
  // Configure for 64x64 panel
  HUB75_I2S_CFG mxconfig(PANEL_WIDTH, PANEL_HEIGHT, 1, _pins);
  mxconfig.gpio.e = 32;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::FM6126A;
  
  Serial.println("Initializing display...");
  display = new MatrixPanel_I2S_DMA(mxconfig);
  
  if (!display->begin()) {
    Serial.println("*** ERROR: Display begin() failed! ***");
    while(1) { delay(1000); }
  }
  
  display->setBrightness8(128);
  display->clearScreen();
  
  Serial.println("Drawing TeachTiles bitmap (INVERTED)...");
  
  // Draw the bitmap - format is 0x00RRGGBB (32-bit ARGB with alpha=0)
  // INVERTED: white becomes black, black becomes white
  for (int y = 0; y < PANEL_HEIGHT; y++) {
    for (int x = 0; x < PANEL_WIDTH; x++) {
      int idx = y * PANEL_WIDTH + x;
      uint32_t pixel = pgm_read_dword(&epd_bitmap_TeachTiles_Graphical_Overlay_v1[idx]);
      
      // Extract RGB and invert (255 - value)
      uint8_t r = 255 - ((pixel >> 16) & 0xFF);
      uint8_t g = 255 - ((pixel >> 8) & 0xFF);
      uint8_t b = 255 - (pixel & 0xFF);
      
      display->drawPixelRGB888(x, y, r, g, b);
    }
  }
  
  Serial.println("Bitmap displayed!");
}

void loop() {
  // Static image - nothing to do
  delay(1000);
}
