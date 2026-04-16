// ==========================================================================
// Inputs.ino — 3 push buttons + squishy-circuits switch
// ==========================================================================
// Simple state-change detector with per-input debounce. Each button exposes
// a single-frame `pressedEdge` flag that is consumed by handleButtonEvents()
// so the main loop never has to worry about repeat events.
// ==========================================================================

struct DebouncedInput {
  uint8_t  pin;
  uint16_t debounceMs;
  bool     lastRaw;
  bool     stable;
  uint32_t lastChangeMs;
  bool     pressedEdge;   // true for exactly one loop after a press
};

static DebouncedInput in_feed    = { BTN_FEED,    BTN_DEBOUNCE_MS,    HIGH, HIGH, 0, false };
static DebouncedInput in_tare    = { BTN_TARE,    BTN_DEBOUNCE_MS,    HIGH, HIGH, 0, false };
static DebouncedInput in_menu    = { BTN_MENU,    BTN_DEBOUNCE_MS,    HIGH, HIGH, 0, false };
static DebouncedInput in_squishy = { SQUISHY_PIN, SQUISHY_DEBOUNCE_MS, HIGH, HIGH, 0, false };

static void tick(DebouncedInput& b) {
  bool raw = digitalRead(b.pin);
  uint32_t now = millis();
  if (raw != b.lastRaw) {
    b.lastRaw      = raw;
    b.lastChangeMs = now;
  }
  if (now - b.lastChangeMs >= b.debounceMs && raw != b.stable) {
    b.stable = raw;
    if (b.stable == LOW) {         // active-low: LOW = pressed
      b.pressedEdge = true;
    }
  }
}

// --------------------------------------------------------------------------
void readButtons() {
  tick(in_feed);
  tick(in_tare);
  tick(in_menu);
  tick(in_squishy);
}

// --------------------------------------------------------------------------
void handleButtonEvents() {
  // --- Manual feed ---------------------------------------------------------
  if (in_feed.pressedEdge) {
    in_feed.pressedEdge = false;
    if (currentState == STATE_IDLE) {
      Serial.println(F("[btn] FEED pressed"));
      startDispense(DEFAULT_PORTION_G, "manual", String(""), String(""), String(""));
    } else {
      Serial.println(F("[btn] FEED ignored (not idle)"));
    }
  }

  // --- Tare ---------------------------------------------------------------
  if (in_tare.pressedEdge) {
    in_tare.pressedEdge = false;
    Serial.println(F("[btn] TARE pressed"));
    scaleTare();
  }

  // --- Display / menu ------------------------------------------------------
  if (in_menu.pressedEdge) {
    in_menu.pressedEdge = false;
    Serial.println(F("[btn] MENU pressed"));
    if (currentState == STATE_ERROR) {
      clearError();
    } else {
      displayNextScreen();
    }
  }

  // --- Squishy Circuits switch --------------------------------------------
  // Treated here as "cat-present confirmation": if idle, request the default
  // portion the same way a pet-owner would via the app.
  if (in_squishy.pressedEdge) {
    in_squishy.pressedEdge = false;
    Serial.println(F("[sqsh] squishy contact detected"));
    if (currentState == STATE_IDLE) {
      startDispense(DEFAULT_PORTION_G, "manual", String(""), String(""), String(""));
    }
  }
}
