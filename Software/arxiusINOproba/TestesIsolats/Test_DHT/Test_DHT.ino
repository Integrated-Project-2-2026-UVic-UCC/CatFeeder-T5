#include "DHT.h"

#define DHTPIN 33
#define DHTTYPE DHT22   // O DHT11 si fos el cas

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println(F("Iniciant test del sensor de temperatura i humitat..."));

  dht.begin();
}

void loop() {
  // El DHT recomana esperar un parell de segons entre mesures.
  delay(2000);

  // Llegim humitat i temp (en Celsius).
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Comprovem si falla per escriure l'error per consola
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Error al llegir el sensor DHT!"));
    return;
  }

  Serial.print(F("Humitat: "));
  Serial.print(h);
  Serial.print(F("%  Temperatura: "));
  Serial.print(t);
  Serial.println(F("°C"));
}
