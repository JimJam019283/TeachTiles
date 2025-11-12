// Pin toggle test for HUB75 inputs
// Cycles each defined HUB75 pin HIGH for 500ms then LOW to allow measurement

#define P_R1_PIN 26
#define P_G1_PIN 27
#define P_B1_PIN 32
#define P_R2_PIN 33
#define P_G2_PIN 25
#define P_B2_PIN 13
#define P_A_PIN 23
#define P_B_PIN 22
#define P_C_PIN 21
#define P_D_PIN 19
#define P_E_PIN 14
#define P_CLK_PIN 18
#define P_LAT_PIN 5
#define P_OE_PIN 15

const int pins[] = {P_R1_PIN, P_G1_PIN, P_B1_PIN, P_R2_PIN, P_G2_PIN, P_B2_PIN,
                    P_A_PIN, P_B_PIN, P_C_PIN, P_D_PIN, P_E_PIN,
                    P_CLK_PIN, P_LAT_PIN, P_OE_PIN};
const char* pinNames[] = {"R1","G1","B1","R2","G2","B2",
                          "A","B","C","D","E",
                          "CLK","LAT","OE"};

void setup() {
  Serial.begin(115200);
  while(!Serial) delay(10);
  Serial.println("Pin toggle test starting");
  for (int i = 0; i < (int)(sizeof(pins)/sizeof(pins[0])); ++i) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
  }
}

void loop() {
  for (int i = 0; i < (int)(sizeof(pins)/sizeof(pins[0])); ++i) {
    Serial.print("Toggling "); Serial.println(pinNames[i]);
    digitalWrite(pins[i], HIGH);
    delay(500);
    digitalWrite(pins[i], LOW);
    delay(200);
  }
  // then blink CLK rapidly for 2 seconds to allow oscilloscope capture
  unsigned long end = millis() + 2000;
  Serial.println("Pulsing CLK for 2s");
  while (millis() < end) {
    digitalWrite(P_CLK_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(P_CLK_PIN, LOW);
    delayMicroseconds(5);
  }
}
