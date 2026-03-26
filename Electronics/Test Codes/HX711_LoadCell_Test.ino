#include <Arduino.h>
#include "HX711.h"

constexpr uint8_t HX711_DOUT_PIN = 4;
constexpr uint8_t HX711_SCK_PIN  = 5;

HX711 scale;

void setup() {
  Serial.begin(115200);
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
}

void loop() {
  if (scale.is_ready()) {
    Serial.print("Raw: ");
    Serial.println(scale.read());
  } else {
    Serial.println("HX711 not ready.");
  }

  delay(500);
}
