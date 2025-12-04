// Simple direct test of LED matrix with proper MatrixPanel_I2S_DMA initialization
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PANEL_RES_X 64
#define PANEL_RES_Y 64
#define PANEL_CHAIN 1

// Monalith pin mapping
#define P_R1_PIN 25
#define P_G1_PIN 26
#define P_B1_PIN 27
#define P_R2_PIN 14
#define P_G2_PIN 12
#define P_B2_PIN 13
#define P_E_PIN 32
#define P_A_PIN 23
#define P_B_PIN 19
#define P_C_PIN 5
#define P_D_PIN 17
#define P_CLK_PIN 16
#define P_LAT_PIN 4
#define P_OE_PIN 15

MatrixPanel_I2S_DMA *matrix = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== Simple Matrix Test ===");
  
  // Create config
  Serial.println("Creating HUB75_I2S_CFG...");
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  
  // Set custom pins
  Serial.println("Setting custom pins...");
  HUB75_I2S_CFG::i2s_pins _pins = {P_R1_PIN, P_G1_PIN, P_B1_PIN, P_R2_PIN, P_G2_PIN, P_B2_PIN,
                                     P_A_PIN, P_B_PIN, P_C_PIN, P_D_PIN, P_E_PIN, P_LAT_PIN, P_OE_PIN, P_CLK_PIN};
  mxconfig.gpio = _pins;
  mxconfig.gpio.e = P_E_PIN;
  
  Serial.println("Creating MatrixPanel_I2S_DMA...");
  matrix = new MatrixPanel_I2S_DMA(mxconfig);
  
  Serial.println("Calling matrix->begin()...");
  if (matrix->begin()) {
    Serial.println("SUCCESS: matrix->begin() returned true!");
  } else {
    Serial.println("ERROR: matrix->begin() returned false!");
    return;
  }
  
  Serial.println("Setting brightness...");
  matrix->setBrightness8(255);
  
  Serial.println("Filling with RED...");
  matrix->fillScreenRGB888(255, 0, 0);
  delay(2000);
  
  Serial.println("Filling with GREEN...");
  matrix->fillScreenRGB888(0, 255, 0);
  delay(2000);
  
  Serial.println("Filling with BLUE...");
  matrix->fillScreenRGB888(0, 0, 255);
  delay(2000);
  
  Serial.println("Filling with WHITE...");
  matrix->fillScreenRGB888(255, 255, 255);
  delay(2000);
  
  Serial.println("Setup complete! Starting color cycle...");
}

void loop() {
  // Cycle through colors
  Serial.println("RED");
  matrix->fillScreenRGB888(255, 0, 0);
  delay(1000);
  
  Serial.println("GREEN");
  matrix->fillScreenRGB888(0, 255, 0);
  delay(1000);
  
  Serial.println("BLUE");
  matrix->fillScreenRGB888(0, 0, 255);
  delay(1000);
  
  Serial.println("WHITE");
  matrix->fillScreenRGB888(255, 255, 255);
  delay(1000);
}
