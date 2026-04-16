// ==========================================================================
// Motor.ino — NEMA 17 via DRV8825 / A4988 (STEP / DIR / ENABLE)
// ==========================================================================
// Closed-loop (gravimetric) dispensing:
//   1) Record tray baseline weight.
//   2) Enable driver and run stepper at constant speed.
//   3) Each loop iteration, read the scale and update cycle.dispensedG.
//   4) Stop when target ± tolerance is reached OR the safety timeout fires.
// AccelStepper.run() is called from the main loop to keep motion smooth.
// ==========================================================================

void motorInit() {
  stepper.setEnablePin(STEPPER_EN);
  stepper.setPinsInverted(false, false, true);   // EN active-LOW
  stepper.setMaxSpeed(STEPPER_MAX_SPEED);
  stepper.setAcceleration(STEPPER_ACCEL);
  stepper.disableOutputs();
  Serial.println(F("[motor] stepper driver ready"));
}

void motorEnable(bool on) {
  if (on) stepper.enableOutputs();
  else    stepper.disableOutputs();
}

// Emergency-stop: cuts current immediately and clears any motion.
void motorEmergencyStop() {
  stepper.stop();
  stepper.setCurrentPosition(stepper.currentPosition());
  stepper.disableOutputs();
  cycle.active = false;
}

// --------------------------------------------------------------------------
// Starts a dispensing cycle. Validates the target and transitions into
// STATE_DISPENSING. The heavy lifting happens in runDispensingCycle().
bool startDispense(float grams,
                   const char* trigger,
                   const String& commandId,
                   const String& catId,
                   const String& scheduleId) {
  if (currentState == STATE_DISPENSING || cycle.active) {
    Serial.println(F("[motor] cycle rejected: already dispensing"));
    return false;
  }
  if (grams < MIN_PORTION_G || grams > MAX_PORTION_G) {
    Serial.printf("[motor] cycle rejected: portion %.1f out of range\n", grams);
    return false;
  }

  cycle.active        = true;
  cycle.targetG       = grams;
  cycle.dispensedG    = 0.0f;
  cycle.baselineG     = telemetry.weightG;
  cycle.startMs       = millis();
  cycle.lastPublishMs = 0;
  cycle.trigger       = trigger;
  cycle.commandId     = commandId;
  cycle.catId         = catId;
  cycle.scheduleId    = scheduleId;

  motorEnable(true);
  stepper.setSpeed(STEPPER_DISPENSE_SPEED);

  currentState = STATE_DISPENSING;
  Serial.printf("[motor] dispense %.1f g (trigger=%s)\n", grams, trigger);
  return true;
}

// --------------------------------------------------------------------------
// Called every loop while in STATE_DISPENSING.
void runDispensingCycle() {
  // Constant-speed run (no target position; we stop on weight).
  stepper.runSpeed();

  const uint32_t now = millis();

  // Dispensed grams come directly from the load cell, relative to baseline.
  cycle.dispensedG = telemetry.weightG - cycle.baselineG;
  if (cycle.dispensedG < 0) cycle.dispensedG = 0;

  // --- 1) Target reached? --------------------------------------------------
  if (cycle.dispensedG >= cycle.targetG - FEED_TOLERANCE_G) {
    motorEmergencyStop();
    cycleFinish("completed", cycle.dispensedG);
    return;
  }

  // --- 2) Safety timeout — prevents overrun / blockage ---------------------
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

  // --- 3) Stream live weight to the cloud ---------------------------------
  if (now - cycle.lastPublishMs >= REALTIME_WEIGHT_INTERVAL_MS) {
    cycle.lastPublishMs = now;
    pushRealtimeWeight(cycle.dispensedG);
  }
}
