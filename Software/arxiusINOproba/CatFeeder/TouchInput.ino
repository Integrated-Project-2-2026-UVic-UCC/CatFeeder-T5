// ==========================================================================
// TouchInput.ino — XPT2046 resistive touch input (NO rendering)
// ==========================================================================
// Replaces the old physical-button module: this hardware has NO buttons, the
// only user interface is the touch panel (shared SPI bus with the TFT).
//
// Screen layout (landscape 320x240, rotation 1):
//   - Left half  (tx <  160) -> MANUAL mode. Holding a finger here dispenses.
//   - Right half (tx >= 160) -> AUTO mode (follow downloaded schedules).
//
// This module ONLY reads touch and updates shared state. Drawing is deferred
// to the display phase.
// ==========================================================================

// --------------------------- Public state ---------------------------------
// UIMode is declared in config.h so all tabs see it. These globals are the
// real definitions; CatFeeder.ino declares matching `extern`s.
UIMode   uiRequestedMode   = UI_MODE_AUTO;  // default to scheduled feeding
bool     uiManualFeedActive = false;        // true while finger is on the dispense (manual) zone

bool     touchDetected = false;             // a valid touch happened this frame
uint16_t touchX = 0;                        // last calibrated X (pixels)
uint16_t touchY = 0;                        // last calibrated Y (pixels)

// Horizontal split between the manual (left) and auto (right) zones.
#define TOUCH_ZONE_SPLIT_X 160

// --------------------------------------------------------------------------
void touchInputInit() {
  // tft.init()/setRotation() may already have run in displayInit(); calling
  // them again is harmless and guarantees the touch calibration is applied.
  tft.init();
  tft.setRotation(1); // landscape 320x240

  // Default calibration { xMin, xMax, yMin, yMax, rotationFlag }.
  // Re-run the TFT_eSPI Touch_calibrate example for pixel-accurate values.
  uint16_t calData[5] = { 701, 551, 491, 197, 4 };
  tft.setTouch(calData);

  Serial.println(F("[touch] XPT2046 input ready"));
}

// --------------------------------------------------------------------------
// Polled every loop. Updates touchDetected / touchX / touchY, the requested
// UI mode and the manual-feed flag. Non-blocking.
void touchInputUpdate() {
  uint16_t tx = 0, ty = 0;
  bool pressed = tft.getTouch(&tx, &ty);

  touchDetected = pressed;

  if (!pressed) {
    // Finger lifted -> stop requesting a manual feed.
    uiManualFeedActive = false;
    return;
  }

  touchX = tx;
  touchY = ty;

  if (tx < TOUCH_ZONE_SPLIT_X) {
    // Left zone: manual dispense. Active for as long as the finger is down.
    uiRequestedMode    = UI_MODE_MANUAL;
    uiManualFeedActive = true;
  } else {
    // Right zone: auto / scheduled mode.
    uiRequestedMode    = UI_MODE_AUTO;
    uiManualFeedActive = false;
  }
}
