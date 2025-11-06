#include <PxMatrix.h>

// Pin mapping (kept in sync with monalith/src/monalith.cpp)
#define P_R1_PIN 25
#define P_G1_PIN 26
#define P_B1_PIN 27
#define P_R2_PIN 14
#define P_G2_PIN 12
#define P_B2_PIN 13

#define P_A_PIN 23
#define P_B_PIN 22
#define P_C_PIN 5
#define P_D_PIN 19
#define P_E_PIN 32

#define P_CLK_PIN 18
#define P_LAT_PIN 4
#define P_OE_PIN 15

#define MATRIX_WIDTH 64
#define MATRIX_HEIGHT 64

// Adjust to match your panel and PxMatrix options
PxMATRIX matrix(MATRIX_WIDTH, MATRIX_HEIGHT, P_LAT_PIN, P_OE_PIN, P_A_PIN, P_B_PIN, P_C_PIN, P_D_PIN, P_E_PIN);

void setup() {
  Serial.begin(115200);
  // Initialize display
  matrix.begin();
  matrix.setMuxPattern(0); // default pattern
  matrix.setPanelIntensity(128); // mid brightness
  matrix.fillScreen(0xF800); // RGB565 red (0xF800)
  matrix.display();
  Serial.println("tmp_full_red: initialized and filled red");
}

void loop() {
  // Keep displaying solid red
  delay(1000);
}
