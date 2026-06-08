#include "config.h"
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

#define XPT2046_CS    21
#define XPT2046_MOSI  23
#define XPT2046_MISO  19
#define XPT2046_CLK   18

#define TOUCH_RAW_X_MIN         200
#define TOUCH_RAW_X_MAX         3700
#define TOUCH_RAW_Y_MIN         240
#define TOUCH_RAW_Y_MAX         3800

#define TOUCH_INVERT_X          1
#define TOUCH_INVERT_Y          1

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS);

// Global UI state variables defined here (declared in config.h)
UIScreen activeUIScreen  = UI_SCREEN_SCREENSAVER;
UIMode   uiRequestedMode = UI_MODE_MANUAL;
bool     screenDirty     = true;

bool     eyesOpen        = true;
bool     cornerHit[4]    = {false, false, false, false};
uint8_t  cornerHitCount  = 0;

uint32_t lastTouchMs           = 0;
uint32_t lastBlinkMs           = 0;
uint32_t lastPeriodicRedrawMs  = 0;
uint32_t lastManualDynamicMs   = 0;
uint32_t firstCornerMs          = 0;

// Extern functions from other tabs
extern void clearError();

// Internal helpers
static bool readTouchMapped(uint16_t &tx, uint16_t &ty, uint16_t &pressure);
static bool getTouchPoint(uint16_t &tx, uint16_t &ty);
static bool checkCornerTouch(uint16_t tx, uint16_t ty);
void resetUnlockPattern();
static void handleMainMenuTouch(uint16_t tx, uint16_t ty);
static void handleAutoModeTouch(uint16_t tx, uint16_t ty);

void touchInputInit() {
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);
  Serial.println(F("[touchUI] XPT2046 touchscreen initialized"));
}

void touchUIInit() {
  lastTouchMs = millis();
  transitionTo(UI_SCREEN_SCREENSAVER);
  Serial.println(F("[touchUI] UI initialized"));
}

void touchInputUpdate() {
  const uint32_t now = millis();

  uint16_t tx = 0;
  uint16_t ty = 0;
  
  // Edge-triggered clicks:
  bool touchedEdge = getTouchPoint(tx, ty);
  if (touchedEdge) {
    lastTouchMs = now;
  }

  // Clear error on any touch edge
  if (currentState == STATE_ERROR) {
    if (touchedEdge) {
      clearError();
      transitionTo(UI_SCREEN_MAIN);
    }
    return;
  }

  // Continuous touch state for hold button
  uint16_t ctx = 0;
  uint16_t cty = 0;
  uint16_t cpressure = 0;
  bool isCurrentlyTouched = readTouchMapped(ctx, cty, cpressure);

  switch (activeUIScreen) {
    case UI_SCREEN_SCREENSAVER:
      if (touchedEdge) {
        transitionTo(UI_SCREEN_UNLOCK);
      }
      break;

    case UI_SCREEN_UNLOCK:
      if (cornerHitCount > 0 &&
          CORNER_TIMEOUT_MS > 0 &&
          (now - firstCornerMs) > CORNER_TIMEOUT_MS) {
        resetUnlockPattern();
        screenDirty = true;
        drawUnlockScreen();
        break;
      }

      if (touchedEdge) {
        bool patternComplete = checkCornerTouch(tx, ty);
        screenDirty = true;
        drawUnlockScreen();

        if (patternComplete) {
          delay(250);
          transitionTo(UI_SCREEN_MAIN);
        }
      }

      if (now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
        transitionTo(UI_SCREEN_SCREENSAVER);
      }
      break;

    case UI_SCREEN_MAIN:
      if (touchedEdge) {
        handleMainMenuTouch(tx, ty);
      }

      if (now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
        transitionTo(UI_SCREEN_SCREENSAVER);
      }
      break;

    case UI_SCREEN_MANUAL_FEED:
      // Back button uses edge-triggered touch
      if (touchedEdge && tx < MANUAL_BACK_W && ty < HEADER_H) {
        transitionTo(UI_SCREEN_MAIN);
        break;
      }

      // Feed button uses continuous hold detection
      if (isCurrentlyTouched) {
        lastTouchMs = now; // keep screen active while held
        if (ctx >= MANUAL_BTN_X &&
            ctx <= MANUAL_BTN_X + MANUAL_BTN_W &&
            cty >= MANUAL_BTN_Y &&
            cty <= MANUAL_BTN_Y + MANUAL_BTN_H) {
          // Pressed inside button
          if (currentState == STATE_IDLE && !cycle.active) {
            // Start dispensing continuously (using safety maximum)
            startDispense(MAX_PORTION_G, "manual", String(""), String(""), String(""));
            screenDirty = true;
          }
        } else {
          // Touch moved outside the button
          if (currentState == STATE_DISPENSING && cycle.active) {
            stopDispense();
            screenDirty = true;
          }
        }
      } else {
        // Not touched
        if (currentState == STATE_DISPENSING && cycle.active) {
          stopDispense();
          screenDirty = true;
        }
      }

      // Allow screensaver transition only when idle
      if (currentState != STATE_DISPENSING && !cycle.active) {
        if (now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
          transitionTo(UI_SCREEN_SCREENSAVER);
        }
      }
      break;

    case UI_SCREEN_AUTO_MODE:
      if (touchedEdge) {
        handleAutoModeTouch(tx, ty);
      }

      if (now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
        transitionTo(UI_SCREEN_SCREENSAVER);
      }
      break;
  }
}

static bool readTouchMapped(uint16_t &tx, uint16_t &ty, uint16_t &pressure) {
  if (!touchscreen.touched()) {
    return false;
  }

  TS_Point p = touchscreen.getPoint();
  if (p.z < TOUCH_Z_THRESHOLD) {
    return false;
  }

  int mappedX = map(p.x, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 1, 320);
  int mappedY = map(p.y, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 1, 240);

  mappedX = constrain(mappedX, 0, 319);
  mappedY = constrain(mappedY, 0, 239);

  tx = (uint16_t)mappedX;
  ty = (uint16_t)mappedY;
  pressure = (uint16_t)p.z;

#if TOUCH_INVERT_X
  tx = 320 - 1 - tx;
#endif

#if TOUCH_INVERT_Y
  ty = 240 - 1 - ty;
#endif

  return true;
}

static bool getTouchPoint(uint16_t &tx, uint16_t &ty) {
  static bool wasTouched = false;
  static uint32_t lastReleaseMs = 0;

  uint16_t pressure = 0;
  bool isTouched = readTouchMapped(tx, ty, pressure);

  if (!isTouched) {
    if (wasTouched) {
      lastReleaseMs = millis();
    }
    wasTouched = false;
    return false;
  }

  // Edge trigger only (one event per touch)
  if (wasTouched) {
    return false;
  }

  if (millis() - lastReleaseMs < 120) {
    return false;
  }

  wasTouched = true;

  Serial.printf("[touch] mapped=(%u,%u) z=%u\n", tx, ty, pressure);
  return true;
}

static bool checkCornerTouch(uint16_t tx, uint16_t ty) {
  int z = CORNER_HIT_ZONE_PX;

  struct Zone {
    int x;
    int y;
  };

  Zone zones[4] = {
    {0,       0},
    {320 - z, 0},
    {0,       240 - z},
    {320 - z, 240 - z}
  };

  const char* names[4] = {"TL", "TR", "BL", "BR"};
  uint8_t expected = cornerHitCount;

  if (expected >= 4) {
    return false;
  }

  Zone &expectedZone = zones[expected];

  Serial.printf("[check] toc px=(%u,%u) esperat=%s zona x=%d..%d y=%d..%d\n",
                tx, ty, names[expected],
                expectedZone.x, expectedZone.x + z,
                expectedZone.y, expectedZone.y + z);

  if (tx >= expectedZone.x && tx < expectedZone.x + z &&
      ty >= expectedZone.y && ty < expectedZone.y + z) {

    if (cornerHitCount == 0) {
      firstCornerMs = millis();
    }

    cornerHit[expected] = true;
    cornerHitCount++;

    Serial.printf("[unlock] corner %u (%s) HIT\n", expected, names[expected]);

    if (cornerHitCount == 4) {
      Serial.println(F("[unlock] pattern complet"));
      return true;
    }
  } else {
    Serial.println(F("[unlock] fora de zona"));
  }

  return false;
}

void resetUnlockPattern() {
  for (int i = 0; i < 4; i++) {
    cornerHit[i] = false;
  }
  cornerHitCount = 0;
  firstCornerMs = 0;
}

static void handleMainMenuTouch(uint16_t tx, uint16_t ty) {
  const int margin = 8;
  const int gap = 8;
  const int cardY = 42;
  const int cardH = 148;
  const int cardW = (320 - margin * 2 - gap) / 2;

  const int manualX = margin;
  const int autoX = margin + cardW + gap;

  if (tx >= manualX && tx <= manualX + cardW &&
      ty >= cardY && ty <= cardY + cardH) {
    uiRequestedMode = UI_MODE_MANUAL;
    transitionTo(UI_SCREEN_MANUAL_FEED);
    return;
  }

  if (tx >= autoX && tx <= autoX + cardW &&
      ty >= cardY && ty <= cardY + cardH) {
    uiRequestedMode = UI_MODE_AUTO;
    transitionTo(UI_SCREEN_AUTO_MODE);
    return;
  }
}

static void handleAutoModeTouch(uint16_t tx, uint16_t ty) {
  if (tx < 95 && ty < HEADER_H) {
    transitionTo(UI_SCREEN_MAIN);
    return;
  }

  if (tx >= 20 && tx <= 300 && ty >= 188 && ty <= 226) {
    uiRequestedMode = UI_MODE_MANUAL;
    transitionTo(UI_SCREEN_MAIN);
    return;
  }
}
