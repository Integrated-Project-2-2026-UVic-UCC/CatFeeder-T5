// ==========================================================================
// Environment.ino — DHT22 + telemetry aggregation
// ==========================================================================

static uint32_t tLastDht = 0;

void sensorsInit() {
  // DHT sensor is currently bypassed; simulating data
  // pinMode(DHT_PIN, INPUT_PULLUP);
  // dht.begin();
  Serial.println(F("[env] DHT22 simulated (sensor bypassed)"));
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
    // Simulated DHT data
    float h = random(400, 500) / 10.0; // 40.0% to 50.0%
    float t = random(225, 236) / 10.0; // 22.5C to 23.5C
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
