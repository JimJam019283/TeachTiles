// Heart Test Display
// Displays a pulsing heart animation on the LED matrix

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

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

// Heart shape (16x14 pixels, will be scaled and centered)
const uint8_t heart[14][16] = {
  {0,0,1,1,1,0,0,0,0,0,1,1,1,0,0,0},
  {0,1,1,1,1,1,0,0,0,1,1,1,1,1,0,0},
  {1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
  {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
  {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
  {0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0},
  {0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0},
  {0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0},
  {0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0},
  {0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0}
};

void drawHeart(int offsetX, int offsetY, int scale, uint8_t r, uint8_t g, uint8_t b) {
  for (int y = 0; y < 14; y++) {
    for (int x = 0; x < 16; x++) {
      if (heart[y][x]) {
        // Draw scaled pixel
        for (int sy = 0; sy < scale; sy++) {
          for (int sx = 0; sx < scale; sx++) {
            int px = offsetX + x * scale + sx;
            int py = offsetY + y * scale + sy;
            if (px >= 0 && px < PANEL_WIDTH && py >= 0 && py < PANEL_HEIGHT) {
              display->drawPixelRGB888(px, py, r, g, b);
            }
          }
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("=== Heart Test Display ===");
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
  
  Serial.println("Heart animation starting!");
}

uint8_t pulsePhase = 0;

void loop() {
  display->clearScreen();
  
  // Pulsing effect - varies between scale 2 and 3
  int scale = (pulsePhase < 128) ? 3 : 2;
  
  // Center the heart
  int heartWidth = 16 * scale;
  int heartHeight = 14 * scale;
  int offsetX = (PANEL_WIDTH - heartWidth) / 2;
  int offsetY = (PANEL_HEIGHT - heartHeight) / 2;
  
  // Pulsing brightness for the red color
  uint8_t brightness = 128 + (sin(pulsePhase * 0.05) * 127);
  
  // Draw a red heart
  drawHeart(offsetX, offsetY, scale, brightness, 0, 0);
  
  pulsePhase += 4;
  delay(30);
}
