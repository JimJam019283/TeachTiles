// Direct ESP32 HUB75 DMA Test - Plasma Effect
// Uses ESP32-HUB75-MatrixPanel-I2S-DMA library directly

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Panel size
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64

// Pin mapping - using your verified pins
// The order in HUB75_I2S_CFG::i2s_pins is: {R1,G1,B1,R2,G2,B2,A,B,C,D,E,LAT,OE,CLK}
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
  32,  // E (needed for 64-row panels)
  4,   // LAT
  15,  // OE
  16   // CLK
};

MatrixPanel_I2S_DMA *display = nullptr;

// Plasma animation variables
uint16_t time_counter = 0;

// Fast 8-bit sine approximation
uint8_t sin8(uint8_t x) {
  static const uint8_t PROGMEM sintable[256] = {
    128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
    176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
    218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
    245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
    255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
    245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
    218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
    176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
    128,124,121,118,115,112,109,106,103,100,97,93,90,88,85,82,
    79,76,73,70,67,65,62,59,57,54,52,49,47,44,42,40,
    37,35,33,31,29,27,25,23,21,20,18,17,15,14,12,11,
    10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,
    0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,
    10,11,12,14,15,17,18,20,21,23,25,27,29,31,33,35,
    37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,
    79,82,85,88,90,93,97,100,103,106,109,112,115,118,121,124
  };
  return pgm_read_byte(&sintable[x]);
}

// Convert HSV to RGB
void hsv2rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b) {
  if (s == 0) {
    r = g = b = v;
    return;
  }
  uint8_t region = h / 43;
  uint8_t remainder = (h - (region * 43)) * 6;
  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
  switch (region) {
    case 0:  r = v; g = t; b = p; break;
    case 1:  r = q; g = v; b = p; break;
    case 2:  r = p; g = v; b = t; break;
    case 3:  r = p; g = q; b = v; break;
    case 4:  r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("=== Plasma Effect Test ===");
  Serial.println("========================================");
  
  // Configure for 64x64 panel
  HUB75_I2S_CFG mxconfig(PANEL_WIDTH, PANEL_HEIGHT, 1, _pins);
  
  // Critical settings for 64x64 panels
  mxconfig.gpio.e = 32;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::FM6126A;
  
  Serial.println("Initializing display...");
  display = new MatrixPanel_I2S_DMA(mxconfig);
  
  if (!display->begin()) {
    Serial.println("*** ERROR: Display begin() failed! ***");
    while(1) { delay(1000); }
  }
  
  display->setBrightness8(128);  // 50% brightness to reduce power
  display->clearScreen();
  
  Serial.println("Plasma effect running!");
}

void loop() {
  // Draw plasma effect
  for (int y = 0; y < PANEL_HEIGHT; y++) {
    for (int x = 0; x < PANEL_WIDTH; x++) {
      // Classic plasma algorithm using multiple sine waves
      uint8_t v1 = sin8(x * 8 + time_counter);
      uint8_t v2 = sin8(y * 8 + time_counter);
      uint8_t v3 = sin8((x + y) * 4 + time_counter);
      uint8_t v4 = sin8(sqrt(x*x + y*y) * 4 + time_counter);
      
      // Combine waves
      uint8_t hue = (v1 + v2 + v3 + v4) / 4;
      
      // Convert to RGB
      uint8_t r, g, b;
      hsv2rgb(hue, 255, 200, r, g, b);
      
      display->drawPixelRGB888(x, y, r, g, b);
    }
  }
  
  time_counter += 2;  // Animation speed
  
  delay(20);  // ~50 FPS
}
