#include <Arduino.h>

// =========================
// Pin configuration
// =========================
constexpr uint8_t STEP_PIN   = 26;
constexpr uint8_t DIR_PIN    = 27;
constexpr uint8_t ENABLE_PIN = 25;   // Optional

// =========================
// Motion settings
// =========================
constexpr uint16_t STEP_PULSE_US = 800;   // Speed control
constexpr uint16_t STEPS_PER_MOVE = 400;  // Number of steps per movement
constexpr uint16_t PAUSE_MS = 1000;

// =========================
// Helper functions
// =========================
void stepMotor(uint16_t steps, bool direction) {
  digitalWrite(DIR_PIN, direction ? HIGH : LOW);

  for (uint16_t i = 0; i < steps; ++i) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_PULSE_US);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting DRV8825 + NEMA17 test...");

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);

  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);

  // DRV8825 ENABLE is usually active LOW
  digitalWrite(ENABLE_PIN, LOW);

  Serial.println("Driver enabled.");
}

void loop() {
  Serial.println("Rotating clockwise...");
  stepMotor(STEPS_PER_MOVE, true);
  delay(PAUSE_MS);

  Serial.println("Rotating counterclockwise...");
  stepMotor(STEPS_PER_MOVE, false);
  delay(PAUSE_MS);
}
