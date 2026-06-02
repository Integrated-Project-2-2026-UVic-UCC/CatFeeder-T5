// ==========================================================================
// Motor.ino — NEMA 17 via DRV8825 (STEP / DIR / ENABLE, EN active-LOW)
// ==========================================================================
// Gravimetric dispensing with a 90% stop:
//   1) Tare the scale and record the baseline.
//   2) Enable the driver and move the auger "indefinitely".
//   3) Each loop, read the scale and update cycle.dispensedG.
//   4) When the scale reaches FEED_STOP_THRESHOLD (90%) of the target, stop
//      the auger. Food still in the channel keeps falling, so we WAIT
//      FEED_SETTLE_MS (non-blocking) and then read the FINAL dispensed weight.
//   5) A hard safety timeout aborts on blockage / overrun.
//
// AccelStepper.run() is called from the main loop AND here to keep motion
// smooth; nothing in this file blocks.
// ==========================================================================

// Non-blocking "settling" sub-state, used after the auger stops so in-flight
// food can land before we record the final weight.
static bool     waitingFinalRead = false;
static uint32_t settleStartMs    = 0;

void motorInit() {
  pinMode(STEPPER_EN, OUTPUT);
  digitalWrite(STEPPER_EN, HIGH); // disabled (active-LOW)
  stepper.setMaxSpeed(STEPPER_MAX_SPEED);
  stepper.setAcceleration(STEPPER_ACCEL);
  Serial.println(F("[motor] stepper driver ready"));
}

// Direct EN control (active-LOW): on -> LOW (energised), off -> HIGH.
void motorEnable(bool on) {
  digitalWrite(STEPPER_EN, on ? LOW : HIGH);
}

// Emergency stop: cut current immediately and clear any pending motion.
void motorEmergencyStop() {
  stepper.stop();
  stepper.move(0);
  motorEnable(false);
  cycle.active = false;
  waitingFinalRead = false;
}

// --------------------------------------------------------------------------
// Start a dispensing cycle. Validates the target, tares the scale and moves
// to STATE_DISPENSING. The control loop is in runDispensingCycle().
//
// Signature: (grams, trigger, commandId, catId, scheduleId)
//   - commandId : Supabase commands.id (empty for scheduled feeds)
//   - catId     : cat UUID (empty if not applicable)
//   - scheduleId: schedule UUID (empty for manual feeds)
bool startDispense(float grams,
                   const char *trigger,
                   const String &commandId,
                   const String &catId,
                   const String &scheduleId) {
  if (currentState == STATE_DISPENSING || cycle.active) {
    Serial.println(F("[motor] cycle rejected: already dispensing"));
    return false;
  }
  if (grams < MIN_PORTION_G || grams > MAX_PORTION_G) {
    Serial.printf("[motor] cycle rejected: portion %.1f out of range\n", grams);
    return false;
  }

  // Tare so dispensedG is measured from a clean zero.
  scaleTare();

  cycle.active        = true;
  cycle.targetG       = grams;
  cycle.dispensedG    = 0.0f;
  cycle.baselineG     = scaleRead();
  cycle.startMs       = millis();
  cycle.lastPublishMs = 0;
  cycle.trigger       = trigger;
  cycle.commandId     = commandId;
  cycle.catId         = catId;
  cycle.scheduleId    = scheduleId;
  cycle.startedAtIso  = isoTimestamp();

  waitingFinalRead = false;

  motorEnable(true);
  stepper.setMaxSpeed(STEPPER_DISPENSE_SPEED);
  stepper.setAcceleration(STEPPER_ACCEL);
  stepper.move(999999999L); // run "indefinitely"; weight will stop us

  currentState = STATE_DISPENSING;
  Serial.printf("[motor] dispense %.1f g (trigger=%s, stop@%.0f%%)\n",
                grams, trigger, FEED_STOP_THRESHOLD * 100.0f);
  return true;
}

// --------------------------------------------------------------------------
// Called every loop while in STATE_DISPENSING. Fully non-blocking.
void runDispensingCycle() {
  stepper.run(); // also stepped from the main loop; safe to call again

  const uint32_t now = millis();

  // --- Phase B: settling after the auger stopped --------------------------
  if (waitingFinalRead) {
    if (now - settleStartMs >= FEED_SETTLE_MS) {
      // In-flight food has landed; read the true final weight.
      float finalDispensed = scaleRead() - cycle.baselineG;
      if (finalDispensed < 0) finalDispensed = 0;
      waitingFinalRead = false;
      cycleFinish("completed", finalDispensed);
    }
    return; // motor already disabled; just wait out the timer
  }

  // --- Phase A: actively dispensing ---------------------------------------
  float dispensed = scaleRead() - cycle.baselineG;
  if (dispensed < 0) dispensed = 0;
  cycle.dispensedG = dispensed;

  // Stop the auger at 90% of the target; the channel completes the rest.
  if (cycle.dispensedG >= cycle.targetG * FEED_STOP_THRESHOLD) {
    stepper.stop();
    stepper.move(0);
    motorEnable(false);
    waitingFinalRead = true;
    settleStartMs    = now;
    Serial.printf("[motor] reached %.0f%% (%.1f/%.1f g) — settling %ums\n",
                  FEED_STOP_THRESHOLD * 100.0f, cycle.dispensedG,
                  cycle.targetG, (unsigned)FEED_SETTLE_MS);
    return;
  }

  // Safety timeout — prevents overrun / blockage.
  if (now - cycle.startMs > STEPPER_MAX_RUN_MS) {
    motorEmergencyStop();
    if (cycle.dispensedG > 0.5f) {
      cycleFinish("partial", cycle.dispensedG);
    } else {
      cycleFinish("error", cycle.dispensedG);
      goToError("Motor timeout");
    }
    return;
  }
}
