// ==========================================================================
// Display.ino — ILI9488 3.5" SPI TFT, OUTPUT ONLY
// ==========================================================================
// The touch panel is intentionally not used in this base code.
// All rendering uses TFT_eSPI. Configure the driver / pins in User_Setup.h.
// ==========================================================================

// Simple palette (RGB565 via TFT_eSPI colour helpers).
#define COL_BG        TFT_BLACK
#define COL_CARD      0x2124            // dark slate
#define COL_ACCENT    TFT_CYAN
#define COL_OK        TFT_GREEN
#define COL_WARN      TFT_ORANGE
#define COL_ERROR     TFT_RED
#define COL_TEXT      TFT_WHITE
#define COL_MUTED     0x94B2            // grey

// Forward declarations of the three screens.
static void drawScreenDashboard();
static void drawScreenSensors();
static void drawScreenNetwork();
static void drawErrorScreen();
static void drawHeader();
static void drawProgress(int x, int y, int w, int h, float value, float maximum);

// --------------------------------------------------------------------------
void displayInit() {
  tft.init();
  tft.setRotation(DISPLAY_ROTATION);   // landscape 480x320
  tft.fillScreen(COL_BG);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  Serial.println(F("[display] ILI9488 initialised"));
}

// --------------------------------------------------------------------------
void displaySplash(const char* text) {
  tft.fillScreen(COL_BG);
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(3);
  tft.drawString("CatFeeder", tft.width() / 2, tft.height() / 2 - 30);
  tft.setTextSize(2);
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.drawString(text, tft.width() / 2, tft.height() / 2 + 10);
  tft.setTextSize(1);
  tft.drawString("fw " FW_VERSION, tft.width() / 2, tft.height() - 14);
  tft.setTextDatum(TL_DATUM);
}

// --------------------------------------------------------------------------
// Called ~4 Hz by the main loop. Draws only the active screen.
void displayUpdate() {
  if (currentState == STATE_ERROR) {
    drawErrorScreen();
    return;
  }
  switch (activeScreen) {
    case 0: drawScreenDashboard(); break;
    case 1: drawScreenSensors();   break;
    case 2: drawScreenNetwork();   break;
  }
}

// Called by Inputs.ino when the user presses BTN_MENU.
void displayNextScreen() {
  activeScreen = (activeScreen + 1) % 3;
  tft.fillScreen(COL_BG);  // wipe residue from previous screen
}

// --------------------------------------------------------------------------
static void drawHeader() {
  tft.fillRect(0, 0, tft.width(), 28, COL_CARD);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(COL_ACCENT, COL_CARD);
  tft.setTextSize(2);
  tft.drawString("CatFeeder", 8, 14);

  // RTC clock on the right.
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(COL_TEXT, COL_CARD);
  char buf[24];
  if (telemetry.rtcOk) {
    DateTime now = rtc.now();
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u",
             now.hour(), now.minute(), now.second());
  } else {
    snprintf(buf, sizeof(buf), "--:--:--");
  }
  tft.drawString(buf, tft.width() - 8, 14);

  tft.setTextDatum(TL_DATUM);
}

// --------------------------------------------------------------------------
static void drawScreenDashboard() {
  drawHeader();

  // Big weight read-out.
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setTextSize(2);
  tft.drawString("Weight", 12, 44);

  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setTextSize(6);
  char buf[16];
  snprintf(buf, sizeof(buf), "%6.1f g", telemetry.weightG);
  tft.drawString(buf, 12, 70);

  // Status line.
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

  // Progress bar when dispensing.
  if (cycle.active) {
    drawProgress(12, 190, tft.width() - 24, 24, cycle.dispensedG, cycle.targetG);
    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT, COL_BG);
    snprintf(buf, sizeof(buf), "%.1f / %.1f g", cycle.dispensedG, cycle.targetG);
    tft.drawString(buf, 12, 222);
  } else {
    // Clear area to avoid ghosting when cycle finishes.
    tft.fillRect(0, 180, tft.width(), 70, COL_BG);
    tft.setTextSize(2);
    tft.setTextColor(COL_MUTED, COL_BG);
    tft.drawString("Press FEED to dispense", 12, 200);
  }

  // Footer: environment summary.
  tft.fillRect(0, 280, tft.width(), 40, COL_CARD);
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_CARD);
  if (isnan(telemetry.temperatureC) || isnan(telemetry.humidity)) {
    tft.drawString("T: --  H: --", 12, 292);
  } else {
    snprintf(buf, sizeof(buf), "T: %.1fC  H: %.0f%%",
             telemetry.temperatureC, telemetry.humidity);
    tft.drawString(buf, 12, 292);
  }
  tft.setTextDatum(MR_DATUM);
  tft.drawString(telemetry.wifiUp ? "WiFi" : "no-net",
                 tft.width() - 8, 298);
  tft.setTextDatum(TL_DATUM);
}

// --------------------------------------------------------------------------
static void drawScreenSensors() {
  drawHeader();
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG);

  char line[48];
  int y = 48;
  tft.drawString("Sensors", 12, y); y += 30;

  tft.setTextColor(COL_MUTED, COL_BG);
  tft.drawString("Load cell (HX711)", 12, y); y += 22;
  snprintf(line, sizeof(line), "  weight     : %6.1f g", telemetry.weightG);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(line, 12, y); y += 24;

  tft.setTextColor(COL_MUTED, COL_BG);
  tft.drawString("Environment (DHT22)", 12, y); y += 22;
  tft.setTextColor(COL_TEXT, COL_BG);
  snprintf(line, sizeof(line), "  temperature: %5.1f C", telemetry.temperatureC);
  tft.drawString(line, 12, y); y += 22;
  snprintf(line, sizeof(line), "  humidity   : %5.1f %%", telemetry.humidity);
  tft.drawString(line, 12, y); y += 26;

  tft.setTextColor(COL_MUTED, COL_BG);
  tft.drawString("RTC (DS3231)", 12, y); y += 22;
  tft.setTextColor(COL_TEXT, COL_BG);
  if (telemetry.rtcOk) {
    DateTime n = rtc.now();
    snprintf(line, sizeof(line), "  %04u-%02u-%02u %02u:%02u:%02u",
             n.year(), n.month(), n.day(),
             n.hour(), n.minute(), n.second());
  } else {
    snprintf(line, sizeof(line), "  not available");
  }
  tft.drawString(line, 12, y);
}

// --------------------------------------------------------------------------
static void drawScreenNetwork() {
  drawHeader();
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG);

  int y = 48;
  tft.drawString("Network", 12, y); y += 30;

  char line[64];
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.drawString("WiFi", 12, y); y += 22;
  tft.setTextColor(COL_TEXT, COL_BG);
  snprintf(line, sizeof(line), "  status : %s",
           WiFi.status() == WL_CONNECTED ? "connected" : "down");
  tft.drawString(line, 12, y); y += 22;
  snprintf(line, sizeof(line), "  SSID   : %s", WIFI_SSID);
  tft.drawString(line, 12, y); y += 22;
  snprintf(line, sizeof(line), "  IP     : %s",
           WiFi.localIP().toString().c_str());
  tft.drawString(line, 12, y); y += 22;
  snprintf(line, sizeof(line), "  RSSI   : %d dBm", WiFi.RSSI());
  tft.drawString(line, 12, y); y += 26;

  tft.setTextColor(COL_MUTED, COL_BG);
  tft.drawString("Device", 12, y); y += 22;
  tft.setTextColor(COL_TEXT, COL_BG);
  snprintf(line, sizeof(line), "  id : %.8s...", DEVICE_ID);
  tft.drawString(line, 12, y); y += 22;
  snprintf(line, sizeof(line), "  fw : %s", FW_VERSION);
  tft.drawString(line, 12, y);
}

// --------------------------------------------------------------------------
static void drawErrorScreen() {
  tft.fillScreen(COL_ERROR);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COL_TEXT, COL_ERROR);
  tft.setTextSize(3);
  tft.drawString("! ERROR", tft.width() / 2, 80);
  tft.setTextSize(2);
  tft.drawString(lastErrorMessage,   tft.width() / 2, 150);
  tft.drawString("Press MENU to reset", tft.width() / 2, 250);
  tft.setTextDatum(TL_DATUM);
}

// --------------------------------------------------------------------------
static void drawProgress(int x, int y, int w, int h,
                         float value, float maximum) {
  if (maximum <= 0) maximum = 1;
  float frac = value / maximum;
  if (frac < 0) frac = 0;
  if (frac > 1) frac = 1;
  int filled = (int)(w * frac);

  tft.drawRect(x, y, w, h, COL_TEXT);
  tft.fillRect(x + 1, y + 1, w - 2, h - 2, COL_BG);
  tft.fillRect(x + 1, y + 1, filled - 2 > 0 ? filled - 2 : 0, h - 2, COL_ACCENT);
}
