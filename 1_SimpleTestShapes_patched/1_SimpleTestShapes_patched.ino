
// Example sketch which shows how to display some patterns
// on a 64x32 LED matrix
//

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// Include Monalith API and the generated bitmap
#include "../monalith/src/monalith.h"
#include "example_bitmap.c"
// Include monalith implementation directly so the sketch links the display helpers
#include "../monalith/src/monalith.cpp"


#define PANEL_RES_X 64      // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 64     // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1      // Total number of panels chained one to another
 
//MatrixPanel_I2S_DMA dma_display;
// Use Monalith's internal PxMatrix instance instead of creating a second
// MatrixPanel_I2S_DMA instance here which can conflict and cause striping.
// The Monalith library manages its own `matrix` object.
uint16_t myBLACK, myWHITE, myRED, myGREEN, myBLUE;
uint16_t testBuffer[64*64];  // Global test buffer for color cycling

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
// From: https://gist.github.com/davidegironi/3144efdc6d67e5df55438cc3cba613c8
uint16_t colorWheel(uint8_t pos) {
  auto color565_from_rgb = [](uint8_t r, uint8_t g, uint8_t b)->uint16_t {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  };
  if(pos < 85) {
    return color565_from_rgb(pos * 3, 255 - pos * 3, 0);
  } else if(pos < 170) {
    pos -= 85;
    return color565_from_rgb(255 - pos * 3, 0, pos * 3);
  } else {
    pos -= 170;
    return color565_from_rgb(0, pos * 3, 255 - pos * 3);
  }
}

void drawText(int /*colorWheelOffset*/)
{
  // Text rendering is disabled in this test build to avoid referencing the
  // DMA driver; Monalith's PxMatrix is used for the bitmap display instead.
}


void setup() {

  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== SimpleTestShapes with Debug ===");

  // Module configuration
  // R1=25,G1=26,B1=27,R2=14,G2=12,B2=13,A=23,B=19,C=5,D=17,E=32,CLK=16,LAT=4,OE=15
  // NOTE: the HUB75_I2S_CFG::i2s_pins order is {R1,G1,B1,R2,G2,B2,A,B,C,D,E,LAT,OE,CLK}
  Serial.println("Creating pin configuration...");
  HUB75_I2S_CFG::i2s_pins _pins = {25, 26, 27, 14, 12, 13, 23, 19, 5, 17, 32, 4, 15, 16};
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN, _pins);
  
  // CRITICAL: 64×64 panels need E pin explicitly set for 1/32 scan
  Serial.println("Setting E pin...");
  mxconfig.gpio.e = 32;  // Force E pin to GPIO32

  // Initialize Monalith (this sets up PxMatrix with the pin mapping
  // compiled into the library). Avoid instantiating another driver here
  // as it will fight over the panel signals and produce striping.
  Serial.println("Calling Monalith::init()...");
  Monalith::init();
  Serial.println("Monalith::init() complete!");

  // Set basic colour constants matching RGB565
  myBLACK = 0x0000;
  myWHITE = 0xFFFF;
  myRED = 0xF800;
  myGREEN = 0x07E0;
  myBLUE = 0x001F;

  // Create a test buffer filled with solid color
  
  // Fill with solid red for initial test
  Serial.println("Filling test buffer with red...");
  for (int i = 0; i < 64*64; i++) {
    testBuffer[i] = myRED; // 0xF800
  }
  
  // Display using non-blocking fast method (proven to work)
  Serial.println("Calling Monalith::showStaticBitmapFast()...");
  Monalith::showStaticBitmapFast(testBuffer);
  Serial.println("showStaticBitmapFast complete!");

}

uint8_t wheelval = 0;
void loop() {
  // Cycle through colors every 3 seconds to verify the panel works
  static uint32_t lastChange = 0;
  static uint8_t colorIndex = 0;
  uint16_t colors[] = {myRED, myGREEN, myBLUE, myWHITE};
  
  if (millis() - lastChange > 3000) {
    lastChange = millis();
    colorIndex = (colorIndex + 1) % 4;
    
    // Fill buffer with new color
    for (int i = 0; i < 64*64; i++) {
      testBuffer[i] = colors[colorIndex];
    }
    
    // Display it
    Monalith::showStaticBitmapFast(testBuffer);
    Serial.printf("Color: %d\n", colorIndex);
  }
  
  delay(100);
}
