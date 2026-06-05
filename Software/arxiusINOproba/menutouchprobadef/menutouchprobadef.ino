// ==========================================================================
// CatFeeder UI — ILI9341 2.8" 320x240 + XPT2046 Touch
// TFT_eSPI per dibuixar, XPT2046_Touchscreen per touch
// Mode manual: 1 toc = dispensar 100 g
// ==========================================================================

#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();

// ---------------- TOUCH XPT2046 ----------------

#define XPT2046_CS    21
#define XPT2046_MOSI  23
#define XPT2046_MISO  19
#define XPT2046_CLK   18

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS);

// ---------------- DISPLAY ----------------

#define SCREEN_W                320
#define SCREEN_H                240

// ---------------- COLORS RGB565 ----------------

#define UI_COL_BG               0x0820
#define UI_COL_CARD             0x1082
#define UI_COL_ACCENT           0x07FF
#define UI_COL_OK               0x07E0
#define UI_COL_DANGER           0xF800
#define UI_COL_WARN             0xFD20
#define UI_COL_TEXT             0xFFFF
#define UI_COL_MUTED            0x7BCF
#define UI_COL_OVERLAY          0x0000
#define UI_COL_CAT_LINE         0xC618

// ---------------- UI CONFIG ----------------

#define UI_IDLE_TIMEOUT_MS      30000
#define CAT_BLINK_INTERVAL_MS   3000
#define CORNER_HIT_ZONE_PX      56
#define CORNER_TIMEOUT_MS       5000

#define MAIN_REDRAW_MS          1000
#define AUTO_REDRAW_MS          1000
#define MANUAL_DYNAMIC_MS       250

#define MANUAL_FEED_AMOUNT_G    100.0f

// ---------------- TOUCH CALIBRATION ----------------

#define TOUCH_RAW_X_MIN         200
#define TOUCH_RAW_X_MAX         3700
#define TOUCH_RAW_Y_MIN         240
#define TOUCH_RAW_Y_MAX         3800

#define TOUCH_INVERT_X          1
#define TOUCH_INVERT_Y          1

// ---------------- MANUAL SCREEN LAYOUT ----------------

#define MANUAL_BTN_X            18
#define MANUAL_BTN_Y            146
#define MANUAL_BTN_W            284
#define MANUAL_BTN_H            74

#define MANUAL_BACK_W           96
#define HEADER_H                32
#define FOOTER_H                18

// ==========================================================================
// MOCKS / DUMMIES
// ==========================================================================

struct Telemetry {
  float weightG = 0.0;
  float temperatureC = 22.1;
  float humidity = 45.0;
  bool wifiUp = true;
  bool rtcOk = true;
} telemetry;

struct Cycle {
  bool active = false;
  float dispensedG = 0.0;
  float targetG = 0.0;
} cycle;

enum State {
  STATE_IDLE,
  STATE_DISPENSING,
  STATE_ERROR
};

State currentState = STATE_IDLE;
uint8_t scheduleCount = 2;

struct DateTime {
  uint8_t hour() { return 12; }
  uint8_t minute() { return 34; }
  uint8_t second() { return 56; }
};

struct RTC_Dummy {
  DateTime now() { return DateTime(); }
} rtc;

void startDispense(float amount, String mode, String s1, String s2, String s3) {
  if (cycle.active) {
    return;
  }

  cycle.active = true;
  cycle.targetG = amount;
  cycle.dispensedG = 0.0;
  currentState = STATE_DISPENSING;

  Serial.println(F("[mock] startDispense"));
}

void motorEmergencyStop() {
  cycle.active = false;
  currentState = STATE_IDLE;
  Serial.println(F("[mock] motorEmergencyStop"));
}

void cycleFinish(String status, float amount) {
  cycle.active = false;
  currentState = STATE_IDLE;
  Serial.println(F("[mock] cycleFinish"));
}

// ==========================================================================
// UI STATE
// ==========================================================================

enum UIScreen {
  UI_SCREEN_SCREENSAVER,
  UI_SCREEN_UNLOCK,
  UI_SCREEN_MAIN,
  UI_SCREEN_MANUAL_FEED,
  UI_SCREEN_AUTO_MODE
};

enum UIMode {
  UI_MODE_MANUAL,
  UI_MODE_AUTO
};

UIScreen activeUIScreen  = UI_SCREEN_SCREENSAVER;
UIMode   uiRequestedMode = UI_MODE_MANUAL;

static uint32_t lastTouchMs           = 0;
static uint32_t lastBlinkMs           = 0;
static uint32_t lastPeriodicRedrawMs  = 0;
static uint32_t lastManualDynamicMs   = 0;

static bool eyesOpen    = true;
static bool screenDirty = true;

static const uint8_t CORNER_COUNT = 4;
static bool     cornerHit[CORNER_COUNT] = {false, false, false, false};
static uint8_t  cornerHitCount = 0;
static uint32_t firstCornerMs = 0;

// ==========================================================================
// PROTOTYPES
// ==========================================================================

static void drawScreensaver();
static void drawUnlockScreen();
static void drawMainMenu();
static void drawManualFeedScreen();
static void drawManualFeedDynamic(bool force);
static void drawAutoModeScreen();
static void drawCatFace(int cx, int cy, bool blinkNow);

static bool checkCornerTouch(uint16_t tx, uint16_t ty);
static void resetUnlockPattern();
static void transitionTo(UIScreen next);

static bool getTouchPoint(uint16_t &tx, uint16_t &ty);
static bool readTouchMapped(uint16_t &tx, uint16_t &ty, uint16_t &pressure);

static void handleMainMenuTouch(uint16_t tx, uint16_t ty);
static void handleManualFeedTouch(uint16_t tx, uint16_t ty);
static void handleAutoModeTouch(uint16_t tx, uint16_t ty);

// ==========================================================================
// UI INIT / UPDATE
// ==========================================================================

void touchUIInit() {
  lastTouchMs = millis();
  transitionTo(UI_SCREEN_SCREENSAVER);
  Serial.println(F("[touchUI] initialised"));
}

void touchUIUpdate() {
  const uint32_t now = millis();

  uint16_t tx = 0;
  uint16_t ty = 0;
  bool touched = getTouchPoint(tx, ty);

  if (touched) {
    lastTouchMs = now;
  }

  switch (activeUIScreen) {
    case UI_SCREEN_SCREENSAVER:
      if (now - lastBlinkMs >= CAT_BLINK_INTERVAL_MS) {
        lastBlinkMs = now;
        eyesOpen = !eyesOpen;
        drawCatFace(SCREEN_W / 2, SCREEN_H / 2, !eyesOpen);
      }

      if (touched) {
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

      if (touched) {
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
      if (touched) {
        handleMainMenuTouch(tx, ty);
      }

      if (now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
        transitionTo(UI_SCREEN_SCREENSAVER);
      }

      if (now - lastPeriodicRedrawMs >= MAIN_REDRAW_MS) {
        lastPeriodicRedrawMs = now;
        screenDirty = true;
      }

      if (screenDirty) {
        drawMainMenu();
      }
      break;

    case UI_SCREEN_MANUAL_FEED:
      if (touched) {
        handleManualFeedTouch(tx, ty);
        lastTouchMs = now;
      }

      if (now - lastManualDynamicMs >= MANUAL_DYNAMIC_MS) {
        lastManualDynamicMs = now;
        drawManualFeedDynamic(false);
      }
      break;

    case UI_SCREEN_AUTO_MODE:
      if (touched) {
        handleAutoModeTouch(tx, ty);
        lastTouchMs = now;
      }

      if (now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
        transitionTo(UI_SCREEN_SCREENSAVER);
      }

      if (now - lastPeriodicRedrawMs >= AUTO_REDRAW_MS) {
        lastPeriodicRedrawMs = now;
        screenDirty = true;
      }

      if (screenDirty) {
        drawAutoModeScreen();
      }
      break;
  }
}

static void transitionTo(UIScreen next) {
  activeUIScreen = next;
  screenDirty = true;
  eyesOpen = true;

  lastPeriodicRedrawMs = millis();
  lastManualDynamicMs = millis();

  tft.fillScreen(UI_COL_BG);

  switch (next) {
    case UI_SCREEN_SCREENSAVER:
      drawScreensaver();
      break;

    case UI_SCREEN_UNLOCK:
      resetUnlockPattern();
      drawUnlockScreen();
      break;

    case UI_SCREEN_MAIN:
      drawMainMenu();
      break;

    case UI_SCREEN_MANUAL_FEED:
      drawManualFeedScreen();
      break;

    case UI_SCREEN_AUTO_MODE:
      drawAutoModeScreen();
      break;
  }

  screenDirty = false;
}

// ==========================================================================
// DRAW: SCREENSAVER
// ==========================================================================

static void drawCatFace(int cx, int cy, bool blinkNow) {
  const uint16_t C = UI_COL_CAT_LINE;
  const uint16_t B = UI_COL_OVERLAY;

  tft.fillScreen(UI_COL_OVERLAY);

  tft.drawCircle(cx, cy, 72, C);
  tft.drawCircle(cx, cy, 71, C);

  tft.drawLine(cx - 72, cy - 22, cx - 44, cy - 76, C);
  tft.drawLine(cx - 44, cy - 76, cx - 16, cy - 52, C);
  tft.drawLine(cx - 16, cy - 52, cx - 72, cy - 22, C);

  tft.drawLine(cx + 72, cy - 22, cx + 44, cy - 76, C);
  tft.drawLine(cx + 44, cy - 76, cx + 16, cy - 52, C);
  tft.drawLine(cx + 16, cy - 52, cx + 72, cy - 22, C);

  tft.fillTriangle(cx - 7, cy + 8, cx + 7, cy + 8, cx, cy + 18, C);

  tft.drawLine(cx - 48, cy + 4,  cx - 16, cy + 7,  C);
  tft.drawLine(cx - 48, cy + 14, cx - 16, cy + 13, C);
  tft.drawLine(cx - 48, cy + 24, cx - 16, cy + 19, C);

  tft.drawLine(cx + 48, cy + 4,  cx + 16, cy + 7,  C);
  tft.drawLine(cx + 48, cy + 14, cx + 16, cy + 13, C);
  tft.drawLine(cx + 48, cy + 24, cx + 16, cy + 19, C);

  tft.fillRect(cx - 48, cy - 40, 24, 20, B);
  tft.fillRect(cx + 24, cy - 40, 24, 20, B);

  if (!blinkNow) {
    tft.drawLine(cx - 47, cy - 30, cx - 26, cy - 38, C);
    tft.drawLine(cx - 47, cy - 30, cx - 26, cy - 23, C);
    tft.drawLine(cx + 26, cy - 30, cx + 47, cy - 38, C);
    tft.drawLine(cx + 26, cy - 30, cx + 47, cy - 23, C);

    tft.fillRect(cx - 39, cy - 35, 7, 10, C);
    tft.fillRect(cx + 33, cy - 35, 7, 10, C);
  } else {
    tft.drawLine(cx - 47, cy - 30, cx - 26, cy - 30, C);
    tft.drawLine(cx + 26, cy - 30, cx + 47, cy - 30, C);
  }

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(UI_COL_MUTED, UI_COL_OVERLAY);
  tft.setTextSize(1);
  tft.drawString("Toca per desbloquejar", cx, SCREEN_H - 8);
  tft.setTextDatum(TL_DATUM);
}

static void drawScreensaver() {
  drawCatFace(SCREEN_W / 2, SCREEN_H / 2, false);
  eyesOpen = true;
  lastBlinkMs = millis();
}

// ==========================================================================
// DRAW: UNLOCK
// ==========================================================================

static void drawUnlockScreen() {
  if (screenDirty) {
    tft.fillScreen(UI_COL_BG);
  }

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(2);
  tft.drawString("Desbloqueig", SCREEN_W / 2, 10);

  tft.setTextSize(1);
  tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
  tft.drawString("Toca: TL > TR > BL > BR", SCREEN_W / 2, 35);

  int z = CORNER_HIT_ZONE_PX;

  int xs[4] = {0, SCREEN_W - z, 0, SCREEN_W - z};
  int ys[4] = {0, 0, SCREEN_H - z, SCREEN_H - z};

  const char* labels[4] = {"TL", "TR", "BL", "BR"};

  for (int i = 0; i < 4; i++) {
    uint16_t col = cornerHit[i] ? UI_COL_OK : UI_COL_CARD;

    tft.fillRect(xs[i], ys[i], z, z, col);
    tft.drawRect(xs[i], ys[i], z, z, UI_COL_MUTED);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(cornerHit[i] ? UI_COL_BG : UI_COL_MUTED, col);
    tft.setTextSize(2);
    tft.drawString(labels[i], xs[i] + z / 2, ys[i] + z / 2);
  }

  char buf[16];
  snprintf(buf, sizeof(buf), "%u / 4", cornerHitCount);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_BG);
  tft.setTextSize(3);
  tft.drawString(buf, SCREEN_W / 2, SCREEN_H / 2 + 5);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

// ==========================================================================
// DRAW: MAIN MENU
// ==========================================================================

static void drawMainMenu() {
  tft.fillScreen(UI_COL_BG);

  tft.fillRect(0, 0, SCREEN_W, HEADER_H, UI_COL_CARD);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("CatFeeder", 8, HEADER_H / 2);

  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(uiRequestedMode == UI_MODE_AUTO ? UI_COL_OK : UI_COL_WARN, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString(uiRequestedMode == UI_MODE_AUTO ? "AUTO" : "MANUAL", SCREEN_W - 8, HEADER_H / 2);

  const int margin = 8;
  const int gap = 8;
  const int cardY = 42;
  const int cardH = 148;
  const int cardW = (SCREEN_W - margin * 2 - gap) / 2;

  const int manualX = margin;
  const int autoX = margin + cardW + gap;

  bool isManual = (uiRequestedMode == UI_MODE_MANUAL);
  bool isAuto = (uiRequestedMode == UI_MODE_AUTO);

  uint16_t manualCol = isManual ? UI_COL_ACCENT : UI_COL_CARD;
  uint16_t autoCol = isAuto ? UI_COL_OK : UI_COL_CARD;

  tft.fillRect(manualX, cardY, cardW, cardH, manualCol);
  tft.drawRect(manualX, cardY, cardW, cardH, isManual ? UI_COL_TEXT : UI_COL_MUTED);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(isManual ? UI_COL_BG : UI_COL_TEXT, manualCol);
  tft.setTextSize(2);
  tft.drawString("MANUAL", manualX + cardW / 2, cardY + 14);

  tft.setTextSize(1);
  tft.drawString("Dispensar", manualX + cardW / 2, cardY + 52);
  tft.drawString("100 g", manualX + cardW / 2, cardY + 68);
  tft.drawString("amb 1 toc", manualX + cardW / 2, cardY + 92);

  if (isManual) {
    tft.setTextSize(2);
    tft.drawString("ACTIU", manualX + cardW / 2, cardY + 116);
  }

  tft.fillRect(autoX, cardY, cardW, cardH, autoCol);
  tft.drawRect(autoX, cardY, cardW, cardH, isAuto ? UI_COL_TEXT : UI_COL_MUTED);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(isAuto ? UI_COL_BG : UI_COL_TEXT, autoCol);
  tft.setTextSize(2);
  tft.drawString("AUTO", autoX + cardW / 2, cardY + 14);

  tft.setTextSize(1);
  tft.drawString("Segueix", autoX + cardW / 2, cardY + 52);
  tft.drawString("horaris", autoX + cardW / 2, cardY + 68);
  tft.drawString("Supabase", autoX + cardW / 2, cardY + 92);

  if (isAuto) {
    tft.setTextSize(2);
    tft.drawString("ACTIU", autoX + cardW / 2, cardY + 116);
  }

  tft.fillRect(0, SCREEN_H - 28, SCREEN_W, 28, UI_COL_CARD);

  char fbuf[80];
  snprintf(
    fbuf,
    sizeof(fbuf),
    "%.1fg  %.0fC  %.0f%%  %s",
    telemetry.weightG,
    telemetry.temperatureC,
    telemetry.humidity,
    telemetry.wifiUp ? "WiFi OK" : "No WiFi"
  );

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_MUTED, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString(fbuf, 8, SCREEN_H - 14);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

// ==========================================================================
// DRAW: MANUAL FEED — estàtic + dinàmic
// ==========================================================================

static void drawManualFeedScreen() {
  tft.fillScreen(UI_COL_BG);

  tft.fillRect(0, 0, SCREEN_W, HEADER_H, UI_COL_CARD);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString("< Enrere", 8, HEADER_H / 2);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("MANUAL", SCREEN_W / 2, HEADER_H / 2);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
  tft.setTextSize(1);
  tft.drawString("1 toc = dispensar 100 g", SCREEN_W / 2, 132);

  drawManualFeedDynamic(true);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

static void drawManualFeedDynamic(bool force) {
  static float lastWeight = -9999.0f;
  static bool lastCycleActive = false;
  static float lastDispensed = -9999.0f;
  static State lastState = STATE_ERROR;
  static bool lastWifi = false;

  bool weightChanged = force || fabs(telemetry.weightG - lastWeight) >= 0.1f;
  bool cycleChanged = force || cycle.active != lastCycleActive || fabs(cycle.dispensedG - lastDispensed) >= 0.2f;
  bool statusChanged = force || currentState != lastState || telemetry.wifiUp != lastWifi;

  if (weightChanged) {
    tft.fillRect(0, 44, SCREEN_W, 64, UI_COL_BG);

    char wbuf[20];
    snprintf(wbuf, sizeof(wbuf), "%.1f g", telemetry.weightG);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
    tft.setTextSize(4);
    tft.drawString(wbuf, SCREEN_W / 2, 76);

    tft.setTextSize(1);
    tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
    tft.drawString("pes al plat", SCREEN_W / 2, 112);

    lastWeight = telemetry.weightG;
  }

  if (cycleChanged) {
    tft.fillRect(20, 118, SCREEN_W - 40, 22, UI_COL_BG);

    if (cycle.active) {
      int bx = 30;
      int by = 120;
      int bw = SCREEN_W - 60;
      int bh = 10;

      float frac = (cycle.targetG > 0) ? cycle.dispensedG / cycle.targetG : 0.0f;
      if (frac > 1.0f) frac = 1.0f;

      tft.drawRect(bx, by, bw, bh, UI_COL_MUTED);
      tft.fillRect(bx + 1, by + 1, (int)((bw - 2) * frac), bh - 2, UI_COL_ACCENT);

      char pbuf[24];
      snprintf(pbuf, sizeof(pbuf), "%.1f / %.1f g", cycle.dispensedG, cycle.targetG);

      tft.setTextDatum(MC_DATUM);
      tft.setTextSize(1);
      tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
      tft.drawString(pbuf, SCREEN_W / 2, 138);
    } else {
      tft.setTextDatum(MC_DATUM);
      tft.setTextSize(1);
      tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
      tft.drawString("1 toc = dispensar 100 g", SCREEN_W / 2, 132);
    }

    uint16_t btnCol = cycle.active ? UI_COL_DANGER : UI_COL_ACCENT;

    tft.fillRect(MANUAL_BTN_X, MANUAL_BTN_Y, MANUAL_BTN_W, MANUAL_BTN_H, btnCol);
    tft.drawRect(MANUAL_BTN_X, MANUAL_BTN_Y, MANUAL_BTN_W, MANUAL_BTN_H, UI_COL_TEXT);
    tft.drawRect(MANUAL_BTN_X + 1, MANUAL_BTN_Y + 1, MANUAL_BTN_W - 2, MANUAL_BTN_H - 2, UI_COL_TEXT);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(cycle.active ? UI_COL_TEXT : UI_COL_BG, btnCol);

    if (cycle.active) {
      tft.setTextSize(2);
      tft.drawString("DISPENSANT...", SCREEN_W / 2, MANUAL_BTN_Y + 28);
      tft.setTextSize(1);
      tft.drawString("toca per aturar", SCREEN_W / 2, MANUAL_BTN_Y + 56);
    } else {
      tft.setTextSize(3);
      tft.drawString("DISPENSAR", SCREEN_W / 2, MANUAL_BTN_Y + 28);
      tft.setTextSize(1);
      tft.drawString("100 g", SCREEN_W / 2, MANUAL_BTN_Y + 56);
    }

    lastCycleActive = cycle.active;
    lastDispensed = cycle.dispensedG;
  }

  if (statusChanged) {
    const char* stateStr = "Idle";
    if (currentState == STATE_DISPENSING) stateStr = "Dispensant";
    if (currentState == STATE_ERROR) stateStr = "ERROR";

    tft.fillRect(0, SCREEN_H - FOOTER_H, SCREEN_W, FOOTER_H, UI_COL_CARD);

    char sbuf[48];
    snprintf(sbuf, sizeof(sbuf), "%s   %s", stateStr, telemetry.wifiUp ? "WiFi OK" : "No WiFi");

    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(UI_COL_MUTED, UI_COL_CARD);
    tft.setTextSize(1);
    tft.drawString(sbuf, 8, SCREEN_H - FOOTER_H / 2);

    lastState = currentState;
    lastWifi = telemetry.wifiUp;
  }

  tft.setTextDatum(TL_DATUM);
}

// ==========================================================================
// DRAW: AUTO MODE
// ==========================================================================

static void drawAutoModeScreen() {
  tft.fillScreen(UI_COL_BG);

  tft.fillRect(0, 0, SCREEN_W, HEADER_H, UI_COL_CARD);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString("< Enrere", 8, HEADER_H / 2);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("AUTO", SCREEN_W / 2, HEADER_H / 2);

  tft.fillRect(16, 46, SCREEN_W - 32, 42, UI_COL_OK);
  tft.drawRect(16, 46, SCREEN_W - 32, 42, UI_COL_TEXT);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_BG, UI_COL_OK);
  tft.setTextSize(2);
  tft.drawString("Mode automatic", SCREEN_W / 2, 67);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(2);
  tft.drawString("Horaris", 20, 105);

  tft.setTextSize(1);
  tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
  tft.drawString("Configurats a Supabase", 20, 132);

  tft.setTextSize(2);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_BG);

  char nbuf[32];
  snprintf(nbuf, sizeof(nbuf), "Actius: %u", scheduleCount);
  tft.drawString(nbuf, 20, 154);

  char wbuf[32];
  snprintf(wbuf, sizeof(wbuf), "Pes: %.1f g", telemetry.weightG);
  tft.drawString(wbuf, 170, 154);

  const int btnX = 20;
  const int btnY = 188;
  const int btnW = SCREEN_W - 40;
  const int btnH = 38;

  tft.fillRect(btnX, btnY, btnW, btnH, UI_COL_DANGER);
  tft.drawRect(btnX, btnY, btnW, btnH, UI_COL_TEXT);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_DANGER);
  tft.setTextSize(2);
  tft.drawString("DESACTIVAR", SCREEN_W / 2, btnY + btnH / 2);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

// ==========================================================================
// TOUCH / HANDLERS
// ==========================================================================

static bool checkCornerTouch(uint16_t tx, uint16_t ty) {
  int z = CORNER_HIT_ZONE_PX;

  struct Zone {
    int x;
    int y;
  };

  Zone zones[4] = {
    {0,            0},
    {SCREEN_W - z, 0},
    {0,            SCREEN_H - z},
    {SCREEN_W - z, SCREEN_H - z}
  };

  const char* names[4] = {"TL", "TR", "BL", "BR"};

  uint8_t expected = cornerHitCount;

  if (expected >= CORNER_COUNT) {
    return false;
  }

  Zone &expectedZone = zones[expected];

  Serial.printf(
    "[check] toc px=(%u,%u) esperat=%s zona x=%d..%d y=%d..%d\n",
    tx,
    ty,
    names[expected],
    expectedZone.x,
    expectedZone.x + z,
    expectedZone.y,
    expectedZone.y + z
  );

  if (tx >= expectedZone.x &&
      tx < expectedZone.x + z &&
      ty >= expectedZone.y &&
      ty < expectedZone.y + z) {

    if (cornerHitCount == 0) {
      firstCornerMs = millis();
    }

    cornerHit[expected] = true;
    cornerHitCount++;

    Serial.printf("[unlock] corner %u (%s) HIT\n", expected, names[expected]);

    if (cornerHitCount == CORNER_COUNT) {
      Serial.println(F("[unlock] pattern complet"));
      return true;
    }
  } else {
    Serial.println(F("[unlock] fora de zona"));
  }

  return false;
}

static void resetUnlockPattern() {
  for (int i = 0; i < CORNER_COUNT; i++) {
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
  const int cardW = (SCREEN_W - margin * 2 - gap) / 2;

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

static void handleManualFeedTouch(uint16_t tx, uint16_t ty) {
  if (tx < MANUAL_BACK_W && ty < HEADER_H) {
    transitionTo(UI_SCREEN_MAIN);
    return;
  }

  if (tx >= MANUAL_BTN_X &&
      tx <= MANUAL_BTN_X + MANUAL_BTN_W &&
      ty >= MANUAL_BTN_Y &&
      ty <= MANUAL_BTN_Y + MANUAL_BTN_H) {

    if (cycle.active) {
      motorEmergencyStop();
    } else {
      startDispense(MANUAL_FEED_AMOUNT_G, "manual", "", "", "");
    }

    drawManualFeedDynamic(true);
    return;
  }
}

static void handleAutoModeTouch(uint16_t tx, uint16_t ty) {
  if (tx < 95 && ty < HEADER_H) {
    transitionTo(UI_SCREEN_MAIN);
    return;
  }

  if (tx >= 20 && tx <= SCREEN_W - 20 && ty >= 188 && ty <= 226) {
    uiRequestedMode = UI_MODE_MANUAL;
    transitionTo(UI_SCREEN_MAIN);
    return;
  }
}

static bool readTouchMapped(uint16_t &tx, uint16_t &ty, uint16_t &pressure) {
  if (!touchscreen.touched()) {
    return false;
  }

  TS_Point p = touchscreen.getPoint();

  int mappedX = map(p.x, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 1, SCREEN_W);
  int mappedY = map(p.y, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 1, SCREEN_H);

  mappedX = constrain(mappedX, 0, SCREEN_W - 1);
  mappedY = constrain(mappedY, 0, SCREEN_H - 1);

  tx = (uint16_t)mappedX;
  ty = (uint16_t)mappedY;
  pressure = (uint16_t)p.z;

#if TOUCH_INVERT_X
  tx = SCREEN_W - 1 - tx;
#endif

#if TOUCH_INVERT_Y
  ty = SCREEN_H - 1 - ty;
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

  // Un sol event per cada toc.
  if (wasTouched) {
    return false;
  }

  if (millis() - lastReleaseMs < 120) {
    return false;
  }

  wasTouched = true;

  Serial.printf(
    "[touch] mapped=(%u,%u) z=%u\n",
    tx,
    ty,
    pressure
  );

  return true;
}

// ==========================================================================
// SETUP / LOOP
// ==========================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);

  tft.init();
  tft.setRotation(1);

  touchUIInit();
}

void loop() {
  static uint32_t lastUpdate = 0;
  uint32_t now = millis();

  if (now - lastUpdate >= 60) {
    lastUpdate = now;
    touchUIUpdate();
  }

  if (cycle.active) {
    cycle.dispensedG += 0.5;
    telemetry.weightG = cycle.dispensedG;

    if (cycle.dispensedG >= cycle.targetG) {
      cycleFinish("completed", cycle.dispensedG);
      drawManualFeedDynamic(true);
    }
  }
}