// ==========================================================================
// Scale.ino — 1 kg load cell through an HX711 24-bit ADC
// ==========================================================================
// Two support routines are provided:
//   scaleCalibrate(knownWeightG) — print a calibration factor over Serial
//                                   that can be pasted back into config.h.
//   scaleTare()                  — zero the scale (call with an empty tray).
// ==========================================================================

void scaleInit() {
  scale.begin(HX711_DT, HX711_SCK);
  delay(50);

  if (!scale.is_ready()) {
    Serial.println(F("[scale] HX711 not responding — check wiring"));
    goToError("Scale not found");
    return;
  }

  scale.set_scale(HX711_CALIBRATION_FACTOR);
  scale.tare(HX711_SAMPLES);           // tray must be empty on boot
  Serial.printf("[scale] ready, factor=%.2f\n", HX711_CALIBRATION_FACTOR);
}

// --------------------------------------------------------------------------
// Safe tare — refuses if a dispensing cycle is active.
void scaleTare() {
  if (cycle.active) {
    Serial.println(F("[scale] tare rejected: cycle active"));
    return;
  }
  scale.tare(HX711_SAMPLES);
  Serial.println(F("[scale] tare done"));
}

// --------------------------------------------------------------------------
// Returns grams (averaged). Non-blocking path: if the ADC is not ready we
// return the last cached value so the loop keeps its cadence.
float scaleRead() {
  if (!scale.is_ready()) return telemetry.weightG;
  float g = scale.get_units(HX711_SAMPLES);
  if (fabsf(g) < SCALE_STABLE_THRESHOLD_G) g = 0.0f;   // null-band around 0
  return g;
}

// --------------------------------------------------------------------------
// Calibration helper. Usage from Serial monitor or a debug menu:
//   1) Remove any weight, call scaleTare().
//   2) Place a known reference weight (e.g. 100 g) on the tray.
//   3) Call scaleCalibrate(100.0).
//   4) Copy the printed factor into HX711_CALIBRATION_FACTOR (config.h).
void scaleCalibrate(float knownWeightG) {
  Serial.println(F("[scale] calibrating..."));
  scale.set_scale();                   // factor = 1
  delay(500);
  long raw = scale.get_average(20);
  float newFactor = (float)raw / knownWeightG;
  scale.set_scale(newFactor);
  Serial.printf("[scale] new calibration factor = %.2f\n", newFactor);
  Serial.println(F("[scale] update HX711_CALIBRATION_FACTOR in config.h"));
}
