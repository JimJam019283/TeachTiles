// Text Display - Shows a 6-letter string on the LED Matrix
// Uses built-in GFX font rendering

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Panel size
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64

// The text to display (change this to your 6-letter string)
const char* displayText = "iPhone";

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
  Serial.println("=== Text Display ===");
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
  
  // Set text properties
  display->setTextSize(1);      // Size 1 = 6x8 pixels per character
  display->setTextWrap(false);
  
  // Calculate center position for 6 characters
  // Each char is 6 pixels wide (5 + 1 spacing), so 6 chars = 36 pixels
  int textWidth = 6 * 6;  // 36 pixels
  int startX = (PANEL_WIDTH - textWidth) / 2;  // Center horizontally
  int startY = (PANEL_HEIGHT - 8) / 2;  // Center vertically (8 pixels tall)
  
  // Draw the text in white
  display->setTextColor(display->color565(255, 255, 255));
  display->setCursor(startX, startY);
  display->print(displayText);
  
  Serial.print("Displaying: ");
  Serial.println(displayText);
  Serial.println("Text displayed!");
}

void loop() {
  // Static display - nothing to do
  // You could add scrolling or color cycling here
  delay(1000);
}
