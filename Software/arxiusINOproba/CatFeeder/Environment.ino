// ==========================================================================
// Environment.ino — DHT22 + telemetry aggregation
// ==========================================================================

static uint32_t tLastDht = 0;

void sensorsInit() {
  // Enable internal pull-up resistor. This is critical if the hardware does not
  // have an external pull-up resistor on the DHT22 data line.
  pinMode(DHT_PIN, INPUT_PULLUP);
  dht.begin();
  Serial.println(F("[env] DHT22 started (with internal pull-up)"));
}

// --------------------------------------------------------------------------
// Pulls one reading from each sensor and caches into `telemetry`.
// Scale is sampled every call (cheap); DHT22 only every DHT_READ_INTERVAL_MS.
void updateSensors() {
  // --- Load cell ---
  // Avoid calling the slow, blocking scaleRead() if we are currently dispensing.
  // In STATE_DISPENSING, weight readings are handled directly and non-blockingly
  // by runDispensingCycle() in Motor.ino.
  if (currentState != STATE_DISPENSING) {
    telemetry.weightG = scaleRead();
  }

  // --- DHT22 (rate-limited) ---
  const uint32_t now = millis();
  if (now - tLastDht >= DHT_READ_INTERVAL_MS) {
    tLastDht = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h)) {
      telemetry.humidity = h;
    } else {
      Serial.println(F("[env] DHT22: Failed to read humidity (NAN)"));
    }
    if (!isnan(t)) {
      telemetry.temperatureC = t;
    } else {
      Serial.println(F("[env] DHT22: Failed to read temperature (NAN)"));
    }
  }
}
