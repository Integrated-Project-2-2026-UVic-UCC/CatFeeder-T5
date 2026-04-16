#include "HX711.h"

#define DT_PIN 16
#define SCK_PIN 17

HX711 scale;

void setup() {
  Serial.begin(115200);
  while(!Serial); // Esperar consola
  Serial.println("Iniciant el test de la bàscula (Load Cell)...");

  scale.begin(DT_PIN, SCK_PIN);
  
  if (scale.is_ready()) {
    Serial.println("HX711 encès i preparat.");
    // scale.set_scale(420.0); // Ajusta la teva calibració aquí un cop la sàpigues  
    scale.tare(); // Posa'l a 0 amb el bol buit
  } else {
    Serial.println("No s'ha trobat el mòdul HX711. Comprova cables DT i SCK.");
  }
}

void loop() {
  if (scale.is_ready()) {
    long raw_reading = scale.read();
    // float g_reading = scale.get_units(5); // Si has posat un factor d'escala funciona en grams
    Serial.print("Lectura CRUA (RAW): ");
    Serial.println(raw_reading);
  } else {
    Serial.println("HX711 no respon.");
  }
  delay(1000);
}
