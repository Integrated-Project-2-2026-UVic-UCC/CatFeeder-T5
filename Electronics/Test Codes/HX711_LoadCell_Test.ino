#include <Arduino.h>
#include "DHT.h"

// =========================
// Pin and sensor type
// =========================
constexpr uint8_t DHT_PIN = 15;
constexpr uint8_t DHT_TYPE = DHT22;

// =========================
// DHT instance
// =========================
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Starting DHT22 test...");

  dht.begin();
}

void loop() {
  // Read humidity
  float humidity = dht.readHumidity();

  // Read temperature in Celsius
  float temperatureC = dht.readTemperature();

  // Optional: Fahrenheit
  float temperatureF = dht.readTemperature(true);

  // Check if any reads failed
  if (isnan(humidity) || isnan(temperatureC) || isnan(temperatureF)) {
    Serial.println("ERROR: Failed to read from DHT22 sensor.");
    delay(2000);
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity, 1);
  Serial.println(" %");

  Serial.print("Temperature: ");
  Serial.print(temperatureC, 1);
  Serial.println(" °C");

  Serial.print("Temperature: ");
  Serial.print(temperatureF, 1);
  Serial.println(" °F");

  Serial.println("------------------------");

  delay(2000);
}
