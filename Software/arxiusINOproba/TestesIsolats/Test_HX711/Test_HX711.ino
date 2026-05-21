#include "HX711.h"

#define DT_PIN 16
#define SCK_PIN 17

#define CALIBRATION_FACTOR -2027.0  // Ajusta si cal

HX711 scale;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Iniciant el test de la bàscula (Load Cell)...");

  scale.begin(DT_PIN, SCK_PIN);

  if (scale.is_ready()) {
    Serial.println("HX711 encès i preparat.");
    scale.set_scale(CALIBRATION_FACTOR);
    scale.tare();
    Serial.println("Bàscula tarada i llesta.");
  } else {
    Serial.println("No s'ha trobat el mòdul HX711. Comprova cables DT i SCK.");
  }
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 't' || c == 'T') {
      Serial.println("\n>>> TARA EN PROGRÉS... No toquis la bàscula <<<");
      scale.tare();
      Serial.println(">>> Tara completada. Bàscula posada a zero <<<\n");
    }
  }

  if (scale.is_ready()) {
    float grams = scale.get_units(10);  // Mitjana de 10 lectures amb factor aplicat

    Serial.print("Pes: ");
    Serial.print(grams, 1);  // 1 decimal
    Serial.println(" g");
  } else {
    Serial.println("HX711 no respon.");
  }
  delay(1000);
}
