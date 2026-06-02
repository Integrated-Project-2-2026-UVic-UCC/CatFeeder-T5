// ==========================================================================
// Environment.ino — DHT22 + telemetry aggregation
// ==========================================================================

static uint32_t tLastDht = 0;

void sensorsInit() {
  dht.begin();
  Serial.println(F("[env] DHT22 started"));
}

// --------------------------------------------------------------------------
// Pulls one reading from each sensor and caches into `telemetry`.
// Scale is sampled every call (cheap); DHT22 only every DHT_READ_INTERVAL_MS.
void updateSensors() {
  // --- Load cell ---
  telemetry.weightG = scaleRead();

  // --- DHT22 (rate-limited) ---
  const uint32_t now = millis();
  if (now - tLastDht >= DHT_READ_INTERVAL_MS) {
    tLastDht = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h)) telemetry.humidity     = h;
    if (!isnan(t)) telemetry.temperatureC = t;
  }
}
