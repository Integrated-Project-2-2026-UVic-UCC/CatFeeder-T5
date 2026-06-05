// ==========================================================================
// Display.ino — ILI9341 2.8" 320x240 SPI TFT (+ XPT2046 touch on same bus)
// ==========================================================================
// PHASE NOTE: on-screen rendering is intentionally deferred to a later phase.
// For now this file only brings up the TFT hardware (which the touch
// controller also needs) and leaves displayUpdate() as a no-op.
//
// The previous rich UI is preserved verbatim under the `#if 0` block at the
// bottom so it can be revived when the display phase begins.
// ==========================================================================

// --------------------------------------------------------------------------
// Minimal hardware bring-up. The actual tft.init()/setRotation()/setTouch()
// is performed in touchInputInit() (TouchInput.ino) so that the touch
// calibration is applied right after init. Kept here as a thin wrapper that
// the rest of the firmware can call during setup().
void displayInit() {
  tft.init();
  tft.setRotation(DISPLAY_ROTATION); // landscape 320x240
  tft.fillScreen(TFT_BLACK);
  Serial.println(F("[display] ILI9341 hardware initialised"));
}

// Simple boot message (kept lightweight; safe before the UI phase exists).
void displaySplash(const char *text) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawString("CatFeeder", tft.width() / 2, tft.height() / 2 - 24);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(text, tft.width() / 2, tft.height() / 2 + 16);
  tft.setTextDatum(TL_DATUM);
}

// Renders the real-time weight, clock, and status on the TFT screen.
void displayUpdate() {
  static DeviceState lastState = STATE_BOOT;
  bool stateChanged = (currentState != lastState);
  lastState = currentState;

  // Clear screen and draw static container cards when the state transitions
  if (stateChanged || (millis() < 3000 && !currentDeviceTime.valid)) {
    tft.fillScreen(TFT_BLACK);
    // Header card background (COL_CARD = 0x2124)
    tft.fillRect(0, 0, 320, 32, 0x2124); 
    // Main weight card background (COL_CARD_LIGHT = 0x18C3)
    tft.fillRect(10, 42, 300, 110, 0x18C3); 
  }

  // 1) Render Header
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_CYAN, 0x2124);
  tft.setTextSize(2);
  tft.drawString("CatFeeder T5", 10, 16);

  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(TFT_WHITE, 0x2124);
  char timeBuf[20];
  if (currentDeviceTime.valid) {
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", 
             currentDeviceTime.hour, currentDeviceTime.minute, currentDeviceTime.second);
  } else {
    snprintf(timeBuf, sizeof(timeBuf), "Syncing time...");
  }
  tft.drawString(timeBuf, 310, 16);
  tft.setTextDatum(TL_DATUM);

  // 2) Render Weight Container
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(0x7BEF, 0x18C3); // Grey label text
  tft.setTextSize(2);
  tft.drawString("PES DEL BOL", 160, 60);

  tft.setTextColor(TFT_WHITE, 0x18C3);
  tft.setTextSize(5);
  char weightBuf[20];
  snprintf(weightBuf, sizeof(weightBuf), "%5.1f g", telemetry.weightG);
  tft.drawString(weightBuf, 160, 105);

  // 3) Render Status and Progress Section (at the bottom half)
  if (currentState == STATE_DISPENSING) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("DISPENSANT...", 160, 170);

    // Draw progress bar
    int barW = 260;
    int barH = 16;
    int barX = 30;
    int barY = 190;
    float frac = cycle.dispensedG / (cycle.targetG > 0 ? cycle.targetG : 1.0f);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    tft.drawRect(barX, barY, barW, barH, TFT_WHITE);
    tft.fillRect(barX + 2, barY + 2, barW - 4, barH - 4, TFT_BLACK);
    int fillW = (int)((barW - 4) * frac);
    if (fillW > 0) {
      tft.fillRect(barX + 2, barY + 2, fillW, barH - 4, TFT_CYAN);
    }
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    char progBuf[30];
    snprintf(progBuf, sizeof(progBuf), "%.1f / %.1f g", cycle.dispensedG, cycle.targetG);
    tft.drawString(progBuf, 160, 222);
  } else if (currentState == STATE_ERROR) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(3);
    tft.drawString("ERROR!", 160, 175);
    
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(lastErrorMessage, 160, 210);
  } else {
    // STATE_IDLE
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("SISTEMA A PUNT", 160, 165);

    // Draw DHT22 environmental readings
    tft.setTextSize(2);
    tft.setTextColor(0x7BEF, TFT_BLACK); // Grey text
    char envBuf[40];
    snprintf(envBuf, sizeof(envBuf), "T: %.1f C   H: %.0f %%", 
             isnan(telemetry.temperatureC) ? 0.0f : telemetry.temperatureC, 
             isnan(telemetry.humidity) ? 0.0f : telemetry.humidity);
    tft.drawString(envBuf, 160, 205);
  }
  tft.setTextDatum(TL_DATUM);
}

#if 0
// ==========================================================================
// ===  Legacy rich UI (ILI9488 480x320, button-driven). Disabled.        ===
// ==========================================================================
#define COL_BG        TFT_BLACK
#define COL_CARD      0x2124
#define COL_ACCENT    TFT_CYAN
#define COL_OK        TFT_GREEN
#define COL_WARN      TFT_ORANGE
#define COL_ERROR     TFT_RED
#define COL_TEXT      TFT_WHITE
#define COL_MUTED     0x94B2

static void drawScreenDashboard();
static void drawScreenSensors();
static void drawScreenNetwork();
static void drawErrorScreen();
static void drawHeader();
static void drawProgress(int x, int y, int w, int h, float value, float maximum);

void displayNextScreen() {
  activeScreen = (activeScreen + 1) % 3;
  tft.fillScreen(COL_BG);
}

static void drawHeader() {
  tft.fillRect(0, 0, tft.width(), 28, COL_CARD);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(COL_ACCENT, COL_CARD);
  tft.setTextSize(2);
  tft.drawString("CatFeeder", 8, 14);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(COL_TEXT, COL_CARD);
  char buf[24];
  // RTC removed: show static placeholder instead of live clock.
  snprintf(buf, sizeof(buf), "--:--:--");
  tft.drawString(buf, tft.width() - 8, 14);
  tft.setTextDatum(TL_DATUM);
}

static void drawScreenDashboard() {
  drawHeader();
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setTextSize(2);
  tft.drawString("Weight", 12, 44);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setTextSize(6);
  char buf[16];
  snprintf(buf, sizeof(buf), "%6.1f g", telemetry.weightG);
  tft.drawString(buf, 12, 70);
  const char* stateLabel = "Idle";
  uint16_t stateCol = COL_OK;
  switch (currentState) {
    case STATE_DISPENSING:       stateLabel = "Dispensing"; stateCol = COL_ACCENT; break;
    case STATE_ERROR:            stateLabel = "Error";      stateCol = COL_ERROR;  break;
    case STATE_OFFLINE_DEGRADED: stateLabel = "Offline";    stateCol = COL_WARN;   break;
    default: break;
  }
  tft.setTextSize(2);
  tft.setTextColor(stateCol, COL_BG);
  tft.drawString(stateLabel, 12, 150);
  if (cycle.active) {
    drawProgress(12, 190, tft.width() - 24, 24, cycle.dispensedG, cycle.targetG);
    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT, COL_BG);
    snprintf(buf, sizeof(buf), "%.1f / %.1f g", cycle.dispensedG, cycle.targetG);
    tft.drawString(buf, 12, 222);
  } else {
    tft.fillRect(0, 180, tft.width(), 70, COL_BG);
  }
}

static void drawScreenSensors() { drawHeader(); }
static void drawScreenNetwork() { drawHeader(); }
static void drawErrorScreen() {
  tft.fillScreen(COL_ERROR);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COL_TEXT, COL_ERROR);
  tft.setTextSize(3);
  tft.drawString("! ERROR", tft.width() / 2, 80);
  tft.setTextDatum(TL_DATUM);
}

static void drawProgress(int x, int y, int w, int h, float value, float maximum) {
  if (maximum <= 0) maximum = 1;
  float frac = value / maximum;
  if (frac < 0) frac = 0;
  if (frac > 1) frac = 1;
  int filled = (int)(w * frac);
  tft.drawRect(x, y, w, h, COL_TEXT);
  tft.fillRect(x + 1, y + 1, w - 2, h - 2, COL_BG);
  tft.fillRect(x + 1, y + 1, filled - 2 > 0 ? filled - 2 : 0, h - 2, COL_ACCENT);
}
#endif // legacy UI
