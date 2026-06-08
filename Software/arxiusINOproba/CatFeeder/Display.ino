#include "config.h"

// Prototypes for internal static helpers
static void drawCatFace(int cx, int cy, bool blinkNow);

void displayInit() {
  tft.init();
  tft.setRotation(DISPLAY_ROTATION); // landscape 320x240
  tft.fillScreen(UI_COL_BG);
  Serial.println(F("[display] ILI9341 hardware initialised"));
}

void displaySplash(const char *text) {
  tft.fillScreen(UI_COL_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_BG);
  tft.setTextSize(3);
  tft.drawString("CatFeeder", 160, 96);
  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(2);
  tft.drawString(text, 160, 150);
  tft.setTextDatum(TL_DATUM);
}

void transitionTo(UIScreen next) {
  // If we leave manual feeding screen, make sure dispensing stops immediately
  if (activeUIScreen == UI_SCREEN_MANUAL_FEED && next != UI_SCREEN_MANUAL_FEED) {
    stopDispense();
  }
  
  activeUIScreen = next;
  screenDirty = true;
  eyesOpen = true;

  lastPeriodicRedrawMs = millis();
  lastManualDynamicMs = millis();

  tft.fillScreen(UI_COL_BG);

  switch (next) {
    case UI_SCREEN_SCREENSAVER:
      uiRequestedMode = UI_MODE_AUTO;
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
void drawScreensaver() {
  drawCatFace(160, 120, false);
  eyesOpen = true;
  lastBlinkMs = millis();
}

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
  tft.drawString("Toca per desbloquejar", cx, 240 - 8);
  tft.setTextDatum(TL_DATUM);
}

// ==========================================================================
// DRAW: UNLOCK
// ==========================================================================
void drawUnlockScreen() {
  if (screenDirty) {
    tft.fillScreen(UI_COL_BG);
  }

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(2);
  tft.drawString("Desbloqueig", 160, 10);

  tft.setTextSize(1);
  tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
  tft.drawString("Toca: TL > TR > BL > BR", 160, 35);

  int z = CORNER_HIT_ZONE_PX;

  int xs[4] = {0, 320 - z, 0, 320 - z};
  int ys[4] = {0, 0, 240 - z, 240 - z};

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
  tft.drawString(buf, 160, 120 + 5);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

// ==========================================================================
// DRAW: MAIN MENU
// ==========================================================================
void drawMainMenu() {
  tft.fillScreen(UI_COL_BG);

  tft.fillRect(0, 0, 320, HEADER_H, UI_COL_CARD);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("CatFeeder", 8, HEADER_H / 2);

  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(uiRequestedMode == UI_MODE_AUTO ? UI_COL_OK : UI_COL_WARN, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString(uiRequestedMode == UI_MODE_AUTO ? "AUTO" : "MANUAL", 320 - 8, HEADER_H / 2);

  const int margin = 8;
  const int gap = 8;
  const int cardY = 42;
  const int cardH = 148;
  const int cardW = (320 - margin * 2 - gap) / 2;

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
  tft.drawString("Mantenir", manualX + cardW / 2, cardY + 52);
  tft.drawString("premsat", manualX + cardW / 2, cardY + 68);
  tft.drawString("per alimentar", manualX + cardW / 2, cardY + 92);

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

  tft.fillRect(0, 240 - 28, 320, 28, UI_COL_CARD);

  char fbuf[80];
  snprintf(
    fbuf,
    sizeof(fbuf),
    "%.1fg  %.0fC  %.0f%%  %s",
    telemetry.weightG,
    isnan(telemetry.temperatureC) ? 0.0f : telemetry.temperatureC,
    isnan(telemetry.humidity) ? 0.0f : telemetry.humidity,
    telemetry.wifiUp ? "WiFi OK" : "No WiFi"
  );

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_MUTED, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString(fbuf, 8, 240 - 14);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

// ==========================================================================
// DRAW: MANUAL FEED
// ==========================================================================
void drawManualFeedScreen() {
  tft.fillScreen(UI_COL_BG);

  tft.fillRect(0, 0, 320, HEADER_H, UI_COL_CARD);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString("< Enrere", 8, HEADER_H / 2);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("MANUAL", 160, HEADER_H / 2);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
  tft.setTextSize(1);
  tft.drawString("Mantenir premsat per dispensar", 160, 132);

  drawManualFeedDynamic(true);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

void drawManualFeedDynamic(bool force) {
  static float lastWeight = -9999.0f;
  static bool lastCycleActive = false;
  static float lastDispensed = -9999.0f;
  static DeviceState lastState = STATE_BOOT;
  static bool lastWifi = false;

  bool weightChanged = force || fabs(telemetry.weightG - lastWeight) >= 0.1f;
  bool cycleChanged = force || cycle.active != lastCycleActive || fabs(cycle.dispensedG - lastDispensed) >= 0.2f;
  bool statusChanged = force || currentState != lastState || telemetry.wifiUp != lastWifi;

  if (weightChanged) {
    tft.fillRect(0, 44, 320, 64, UI_COL_BG);

    char wbuf[20];
    snprintf(wbuf, sizeof(wbuf), "%.1f g", telemetry.weightG);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
    tft.setTextSize(4);
    tft.drawString(wbuf, 160, 76);

    tft.setTextSize(1);
    tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
    tft.drawString("pes al plat", 160, 112);

    lastWeight = telemetry.weightG;
  }

  if (cycleChanged) {
    tft.fillRect(20, 118, 320 - 40, 22, UI_COL_BG);

    if (cycle.active) {
      int bx = 30;
      int by = 120;
      int bw = 320 - 60;
      int bh = 10;

      float frac = (cycle.targetG > 0) ? cycle.dispensedG / cycle.targetG : 0.0f;
      if (frac > 1.0f) frac = 1.0f;

      tft.drawRect(bx, by, bw, bh, UI_COL_MUTED);
      tft.fillRect(bx + 1, by + 1, (int)((bw - 2) * frac), bh - 2, UI_COL_ACCENT);

      char pbuf[24];
      snprintf(pbuf, sizeof(pbuf), "%.1f g", cycle.dispensedG);

      tft.setTextDatum(MC_DATUM);
      tft.setTextSize(1);
      tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
      tft.drawString(pbuf, 160, 138);
    } else {
      tft.setTextDatum(MC_DATUM);
      tft.setTextSize(1);
      tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
      tft.drawString("Mantenir premsat per dispensar", 160, 132);
    }

    uint16_t btnCol = cycle.active ? UI_COL_DANGER : UI_COL_ACCENT;

    tft.fillRect(MANUAL_BTN_X, MANUAL_BTN_Y, MANUAL_BTN_W, MANUAL_BTN_H, btnCol);
    tft.drawRect(MANUAL_BTN_X, MANUAL_BTN_Y, MANUAL_BTN_W, MANUAL_BTN_H, UI_COL_TEXT);
    tft.drawRect(MANUAL_BTN_X + 1, MANUAL_BTN_Y + 1, MANUAL_BTN_W - 2, MANUAL_BTN_H - 2, UI_COL_TEXT);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(cycle.active ? UI_COL_TEXT : UI_COL_BG, btnCol);

    if (cycle.active) {
      tft.setTextSize(2);
      tft.drawString("DISPENSANT...", 160, MANUAL_BTN_Y + 28);
      tft.setTextSize(1);
      tft.drawString("deixa anar per aturar", 160, MANUAL_BTN_Y + 56);
    } else {
      tft.setTextSize(3);
      tft.drawString("MANTENIR", 160, MANUAL_BTN_Y + 28);
      tft.setTextSize(1);
      tft.drawString("per dispensar pinso", 160, MANUAL_BTN_Y + 56);
    }

    lastCycleActive = cycle.active;
    lastDispensed = cycle.dispensedG;
  }

  if (statusChanged) {
    const char* stateStr = "Idle";
    if (currentState == STATE_DISPENSING) stateStr = "Dispensant";
    if (currentState == STATE_ERROR) stateStr = "ERROR";

    tft.fillRect(0, 240 - FOOTER_H, 320, FOOTER_H, UI_COL_CARD);

    char sbuf[48];
    snprintf(sbuf, sizeof(sbuf), "%s   %s", stateStr, telemetry.wifiUp ? "WiFi OK" : "No WiFi");

    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(UI_COL_MUTED, UI_COL_CARD);
    tft.setTextSize(1);
    tft.drawString(sbuf, 8, 240 - FOOTER_H / 2);

    lastState = currentState;
    lastWifi = telemetry.wifiUp;
  }

  tft.setTextDatum(TL_DATUM);
}

// ==========================================================================
// DRAW: AUTO MODE
// ==========================================================================
void drawAutoModeScreen() {
  tft.fillScreen(UI_COL_BG);

  tft.fillRect(0, 0, 320, HEADER_H, UI_COL_CARD);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString("< Enrere", 8, HEADER_H / 2);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("AUTO", 160, HEADER_H / 2);

  tft.fillRect(16, 46, 320 - 32, 42, UI_COL_OK);
  tft.drawRect(16, 46, 320 - 32, 42, UI_COL_TEXT);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_BG, UI_COL_OK);
  tft.setTextSize(2);
  tft.drawString("Mode automatic", 160, 67);

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
  const int btnW = 320 - 40;
  const int btnH = 38;

  tft.fillRect(btnX, btnY, btnW, btnH, UI_COL_DANGER);
  tft.drawRect(btnX, btnY, btnW, btnH, UI_COL_TEXT);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_DANGER);
  tft.setTextSize(2);
  tft.drawString("DESACTIVAR", 160, btnY + btnH / 2);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

// ==========================================================================
// displayUpdate: Called periodically from loop
// ==========================================================================
void displayUpdate() {
  const uint32_t now = millis();

  switch (activeUIScreen) {
    case UI_SCREEN_SCREENSAVER:
      if (now - lastBlinkMs >= CAT_BLINK_INTERVAL_MS) {
        lastBlinkMs = now;
        eyesOpen = !eyesOpen;
        drawCatFace(160, 120, !eyesOpen);
      }
      break;

    case UI_SCREEN_UNLOCK:
      // Dynamically redrawn by touch events in TouchInput.ino
      break;

    case UI_SCREEN_MAIN:
      if (now - lastPeriodicRedrawMs >= MAIN_REDRAW_MS) {
        lastPeriodicRedrawMs = now;
        screenDirty = true;
      }
      if (screenDirty) {
        drawMainMenu();
      }
      break;

    case UI_SCREEN_MANUAL_FEED:
      if (now - lastManualDynamicMs >= MANUAL_DYNAMIC_MS) {
        lastManualDynamicMs = now;
        drawManualFeedDynamic(false);
      }
      break;

    case UI_SCREEN_AUTO_MODE:
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
