// Direct PxMatrix Test - Minimal sketch to verify LED matrix works
// Bypasses Monalith library entirely

// Fix for ESP32 Arduino Core 3.x compatibility with PxMatrix
#include <driver/gpio.h>
#include <soc/gpio_struct.h>
#include <PxMatrix.h>

// Panel size
#define WIDTH 64
#define HEIGHT 64

// Pin mapping (verified correct)
#define P_R1 25
#define P_G1 26
#define P_B1 27
#define P_R2 14
#define P_G2 12
#define P_B2 13
#define P_A 23
#define P_B 22  // Fixed: was 19, now 22
#define P_C 5
#define P_D 17
#define P_E 32
#define P_CLK 16
#define P_LAT 4
#define P_OE 15

// Create PxMatrix object
PxMATRIX display(WIDTH, HEIGHT, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

// Timer for display refresh
hw_timer_t* timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR displayISR() {
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(70);
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== Direct PxMatrix Test ===");
  
  // Force control pins to known state
  pinMode(P_OE, OUTPUT);
  pinMode(P_LAT, OUTPUT);
  pinMode(P_CLK, OUTPUT);
  digitalWrite(P_OE, LOW);   // Enable output
  digitalWrite(P_LAT, LOW);
  digitalWrite(P_CLK, LOW);
  
  Serial.printf("Pins: R1=%d G1=%d B1=%d R2=%d G2=%d B2=%d\n", P_R1, P_G1, P_B1, P_R2, P_G2, P_B2);
  Serial.printf("      A=%d B=%d C=%d D=%d E=%d\n", P_A, P_B, P_C, P_D, P_E);
  Serial.printf("      CLK=%d LAT=%d OE=%d\n", P_CLK, P_LAT, P_OE);
  
  // Initialize display with 1/32 scan (for 64x64 panel)
  Serial.println("Initializing PxMatrix...");
  display.begin(32);
  display.setBrightness(255);  // Max brightness
  display.clearDisplay();
  
  // Set up timer for display refresh (4ms = 250Hz refresh)
  Serial.println("Setting up timer...");
  timer = timerBegin(1000000);  // 1MHz
  timerAttachInterrupt(timer, &displayISR);
  timerAlarm(timer, 4000, true, 0);  // 4ms interval, auto-reload
  
  Serial.println("Setup complete! Starting color test...");
}

void loop() {
  static uint8_t phase = 0;
  static uint32_t lastChange = 0;
  
  if (millis() - lastChange > 2000) {
    lastChange = millis();
    
    // Clear display
    display.clearDisplay();
    
    uint16_t color;
    const char* colorName;
    
    switch (phase % 5) {
      case 0: color = 0xF800; colorName = "RED"; break;      // Red
      case 1: color = 0x07E0; colorName = "GREEN"; break;    // Green
      case 2: color = 0x001F; colorName = "BLUE"; break;     // Blue
      case 3: color = 0xFFFF; colorName = "WHITE"; break;    // White
      case 4: color = 0xFFE0; colorName = "YELLOW"; break;   // Yellow
    }
    
    Serial.printf("Filling display with %s (0x%04X)\n", colorName, color);
    
    // Fill entire display with color
    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        display.drawPixel(x, y, color);
      }
    }
    
    phase++;
  }
  
  delay(10);
}
