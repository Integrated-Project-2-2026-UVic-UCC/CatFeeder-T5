// ==========================================================================
// probaMenuTouch.ino — Sketch per provar la UI tàctil per separat
// Basat en READMEpantallatàctil.md i MenuSpecifications.md
// ==========================================================================

#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// --- Orientació i resolució ---
#define SCREEN_W 320
#define SCREEN_H 240

// --- Colors UI (RGB565) ---
#define UI_COL_BG 0x0820
#define UI_COL_CARD 0x1082
#define UI_COL_ACCENT 0x07FF
#define UI_COL_OK 0x07E0
#define UI_COL_DANGER 0xF800
#define UI_COL_WARN 0xFD20
#define UI_COL_TEXT 0xFFFF
#define UI_COL_MUTED 0x7BCF
#define UI_COL_OVERLAY 0x0000
#define UI_COL_CAT_LINE 0xC618

#define UI_IDLE_TIMEOUT_MS 30000
#define CAT_BLINK_INTERVAL_MS 3000
#define CORNER_HIT_ZONE_PX 70
#define CORNER_TIMEOUT_MS 5000
// ---- Calibració manual XPT2046 (raw ADC → píxels) --------------------------
// El teu getTouch() sempre retornava (0,0) perquè els valors de calibració
// { 504, 1, 511, 1, 0 } eren degenerades (offset=1 és físicament impossible).
// Ara llegim directament els valors raw de l'ADC i els mapegem a mà.
//
// COM AJUSTAR: toca les 4 cantonades i anota els raw=(X,Y) del Serial.
// Si toques una cantonada i px no canvia prou, ajusta els MIN/MAX.
// Si els eixos van intercanviats, posa TOUCH_SWAP_XY true.
// Si un eix va al revés, posa el _INV corresponent a true.
//
// Valors estàndard XPT2046 quan funciona correctament sense conflictes
// elèctrics. Un cop corregit el cablejat, pots ajustar aquests límits segons el
// que llegeixis al Serial.
#define TOUCH_RAW_X_MIN 300  // raw X mínim (cantonada esquerra)
#define TOUCH_RAW_X_MAX 3600 // raw X màxim (cantonada dreta)
#define TOUCH_RAW_Y_MIN 300  // raw Y mínim (cantonada superior)
#define TOUCH_RAW_Y_MAX 3600 // raw Y màxim (cantonada inferior)
#define TOUCH_SWAP_XY true   // TRUE necessari per a pantalles horitzontals
#define TOUCH_X_INV false // posa true si l'eix X va invertit (esquerra/dreta)
#define TOUCH_Y_INV false // posa true si l'eix Y va invertit (dalt/baix)
// Histeresi de pressió: Z ha de superar PRESS per iniciar i caure per sota
// RELEASE per acabar
#define TOUCH_Z_PRESS 500 // llindar per detectar inici de toc
#define TOUCH_Z_RELEASE 300 // llindar per detectar final de toc (evita lectures espúries)

// --- Nou: Suport per a calibració nativa de TFT_eSPI (Molt més estable!) ---
#define USE_NATIVE_CALIBRATION true  // Canvia a false per utilitzar mapejat manual
#define TOUCH_CAL_DATA         { 300, 3600, 300, 3600, 7 } // Valors per defecte, es sobreescriuen en calibrar
static uint16_t touchCalData[5] = TOUCH_CAL_DATA;

// ---------- Mocks i Dummies per poder compilar l'UI aïlladament -------------
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

enum State { STATE_IDLE, STATE_DISPENSING, STATE_ERROR };
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

// Dummies per substituir les crides al motor
void startDispense(float amount, String mode, String s1, String s2, String s3) {
  cycle.active = true;
  cycle.targetG = amount;
  cycle.dispensedG = 0.0;
  currentState = STATE_DISPENSING;
  Serial.println(F("[mock] startDispense cridat"));
}

void motorEmergencyStop() {
  cycle.active = false;
  currentState = STATE_IDLE;
  Serial.println(F("[mock] motorEmergencyStop cridat"));
}

void cycleFinish(String status, float amount) {
  cycle.active = false;
  currentState = STATE_IDLE;
  Serial.println(F("[mock] cycleFinish cridat"));
}
// ----------------------------------------------------------------------------

// ---------- Estat de la UI ------------------------------------------------
enum UIScreen {
  UI_SCREEN_SCREENSAVER,
  UI_SCREEN_UNLOCK,
  UI_SCREEN_MAIN,
  UI_SCREEN_MANUAL_FEED,
  UI_SCREEN_AUTO_MODE
};

// Modes de funcionament
enum UIMode { UI_MODE_MANUAL, UI_MODE_AUTO };

// ---------- Variables globals ---------------------------------------------
UIScreen activeUIScreen = UI_SCREEN_SCREENSAVER;
UIMode uiRequestedMode = UI_MODE_MANUAL;
bool uiManualFeedActive = false;

// ---------- Variables internes de la UI -----------------------------------
static uint32_t lastTouchMs = 0;
static uint32_t lastBlinkMs = 0;
static bool eyesOpen = true;
static bool screenDirty = true;

static const uint8_t CORNER_COUNT = 4;
static bool cornerHit[CORNER_COUNT] = {false, false, false, false};
static uint8_t cornerHitCount = 0;
static uint32_t firstCornerMs = 0;

// ---------- Prototypes internes -------------------------------------------
static void drawScreensaver();
static void drawUnlockScreen();
static void drawMainMenu();
static void drawManualFeedScreen();
static void drawAutoModeScreen();
static void drawCatFace(int cx, int cy, bool blinkNow);
static bool checkCornerTouch(uint16_t tx, uint16_t ty);
static void resetUnlockPattern();
static void transitionTo(UIScreen next);
static bool getTouchPoint(uint16_t &tx, uint16_t &ty);
static void handleMainMenuTouch(uint16_t tx, uint16_t ty);
static void handleManualFeedTouch(uint16_t tx, uint16_t ty);
static void handleManualFeedRelease();
static void handleAutoModeTouch(uint16_t tx, uint16_t ty);

void touchUIInit() {
  // Cridem al tàctil natiu de TFT_eSPI si està habilitat
  if (USE_NATIVE_CALIBRATION) {
    tft.setTouch(touchCalData);
  }
  lastTouchMs = millis();
  transitionTo(UI_SCREEN_SCREENSAVER);
  Serial.println(F("[touchUI] initialised"));
}

void touchUIUpdate() {
  const uint32_t now = millis();
  static bool prevTouched = false; // per detectar flanc de pujada

  uint16_t tx = 0, ty = 0;
  bool touched = getTouchPoint(tx, ty);
  bool touchStart =
      touched && !prevTouched; // TRUE només al primer frame de cada toc
  prevTouched = touched;

  if (touchStart)
    lastTouchMs = now; // idle timeout sols conta des del nou toc

  switch (activeUIScreen) {
  case UI_SCREEN_SCREENSAVER:
    if (now - lastBlinkMs >= CAT_BLINK_INTERVAL_MS) {
      lastBlinkMs = now;
      eyesOpen = !eyesOpen;
      drawCatFace(SCREEN_W / 2, SCREEN_H / 2, !eyesOpen);
    }
    if (touchStart) {
      transitionTo(UI_SCREEN_UNLOCK);
    }
    break;

  case UI_SCREEN_UNLOCK:
    if (touchStart) {
      bool patternComplete = checkCornerTouch(tx, ty);
      screenDirty = true;
      if (cornerHitCount > 0 && CORNER_TIMEOUT_MS > 0 &&
          (now - firstCornerMs) > CORNER_TIMEOUT_MS) {
        resetUnlockPattern();
        screenDirty = true;
      }
      if (patternComplete) {
        drawUnlockScreen(); // dibuixa l'estat final abans de fer transició
        delay(300);
        transitionTo(UI_SCREEN_MAIN);
        break;
      }
    }
    if (screenDirty)
      drawUnlockScreen(); // redibuja NOMÉS quan cal
    if (!touched && now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
      transitionTo(UI_SCREEN_SCREENSAVER);
    }
    break;

  case UI_SCREEN_MAIN:
    if (touchStart) {
      handleMainMenuTouch(tx, ty);
    }
    if (!touched && now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
      transitionTo(UI_SCREEN_SCREENSAVER);
    }
    if (now % 1000 < 60)
      screenDirty = true;
    if (screenDirty)
      drawMainMenu();
    break;

  case UI_SCREEN_MANUAL_FEED:
    if (touched) { // ← continu: mantén pressionat per dispensar
      handleManualFeedTouch(tx, ty);
      lastTouchMs = now;
    }
    if (now % 250 < 60)
      screenDirty = true;
    if (screenDirty)
      drawManualFeedScreen();
    break;

  case UI_SCREEN_AUTO_MODE:
    if (touchStart) {
      handleAutoModeTouch(tx, ty);
      lastTouchMs = now;
    }
    if (!touched && now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
      transitionTo(UI_SCREEN_SCREENSAVER);
    }
    if (now % 1000 < 60)
      screenDirty = true;
    if (screenDirty)
      drawAutoModeScreen();
    break;
  }
}

static void transitionTo(UIScreen next) {
  if (activeUIScreen == UI_SCREEN_MANUAL_FEED) {
    uiManualFeedActive = false;
  }
  activeUIScreen = next;
  screenDirty = true;
  eyesOpen = true;
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

static void drawCatFace(int cx, int cy, bool blinkNow) {
  const uint16_t C = UI_COL_CAT_LINE;
  const uint16_t B = UI_COL_OVERLAY;

  tft.drawCircle(cx, cy, 90, C);
  tft.drawCircle(cx, cy, 89, C);

  tft.drawLine(cx - 90, cy - 30, cx - 55, cy - 95, C);
  tft.drawLine(cx - 55, cy - 95, cx - 20, cy - 65, C);
  tft.drawLine(cx - 20, cy - 65, cx - 90, cy - 30, C);

  tft.drawLine(cx + 90, cy - 30, cx + 55, cy - 95, C);
  tft.drawLine(cx + 55, cy - 95, cx + 20, cy - 65, C);
  tft.drawLine(cx + 20, cy - 65, cx + 90, cy - 30, C);

  tft.fillTriangle(cx - 8, cy + 10, cx + 8, cy + 10, cx, cy + 22, C);

  tft.drawLine(cx - 55, cy + 5, cx - 15, cy + 8, C);
  tft.drawLine(cx - 55, cy + 15, cx - 15, cy + 14, C);
  tft.drawLine(cx - 55, cy + 25, cx - 15, cy + 20, C);

  tft.drawLine(cx + 55, cy + 5, cx + 15, cy + 8, C);
  tft.drawLine(cx + 55, cy + 15, cx + 15, cy + 14, C);
  tft.drawLine(cx + 55, cy + 25, cx + 15, cy + 20, C);

  tft.fillRect(cx - 55, cy - 45, 30, 22, B);
  tft.fillRect(cx + 25, cy - 45, 30, 22, B);

  if (!blinkNow) {
    tft.drawLine(cx - 54, cy - 35, cx - 27, cy - 44, C);
    tft.drawLine(cx - 54, cy - 35, cx - 27, cy - 27, C);
    tft.drawLine(cx + 26, cy - 35, cx + 53, cy - 44, C);
    tft.drawLine(cx + 26, cy - 35, cx + 53, cy - 27, C);
    tft.drawLine(cx - 54, cy - 35, cx - 54, cy - 35, C);
    tft.drawLine(cx + 53, cy - 35, cx + 53, cy - 35, C);
    tft.fillRect(cx - 44, cy - 40, 8, 12, C);
    tft.fillRect(cx + 36, cy - 40, 8, 12, C);
  } else {
    tft.drawLine(cx - 54, cy - 35, cx - 26, cy - 35, C);
    tft.drawLine(cx + 26, cy - 35, cx + 54, cy - 35, C);
  }

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(UI_COL_MUTED, UI_COL_OVERLAY);
  tft.setTextSize(1);
  tft.drawString("Toca per desbloquejar", cx, SCREEN_H - 8);
  tft.setTextDatum(TL_DATUM);
}

static void drawScreensaver() {
  tft.fillScreen(UI_COL_OVERLAY);
  drawCatFace(SCREEN_W / 2, SCREEN_H / 2, false);
  eyesOpen = true;
  lastBlinkMs = millis();
}

static void drawUnlockScreen() {
  if (screenDirty) {
    tft.fillScreen(UI_COL_BG);
  }
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(2);
  tft.drawString("Desbloqueig de seguretat", SCREEN_W / 2, 12);
  tft.setTextSize(1);
  tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
  tft.drawString("Toca les 4 cantonades: TL > TR > BL > BR", SCREEN_W / 2, 38);

  int z = CORNER_HIT_ZONE_PX;

  // Les 4 cantonades en ordre: TL, TR, BL, BR
  int xs[4] = {0, SCREEN_W - z, 0, SCREEN_W - z};
  int ys[4] = {0, 0, SCREEN_H - z, SCREEN_H - z};
  const char *labels[4] = {"TL", "TR", "BL", "BR"};

  for (int i = 0; i < 4; i++) {
    bool isDone = (cornerHitCount > i);
    bool isExpected = (cornerHitCount == i);
    uint16_t col =
        isDone ? UI_COL_OK : (isExpected ? UI_COL_ACCENT : UI_COL_CARD);
    uint16_t border = (isDone || isExpected) ? UI_COL_TEXT : UI_COL_MUTED;
    uint16_t txtCol = (isDone || isExpected) ? UI_COL_BG : UI_COL_MUTED;
    tft.fillRect(xs[i], ys[i], z, z, col);
    tft.drawRect(xs[i], ys[i], z, z, border);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(txtCol, col);
    tft.setTextSize(2);
    tft.drawString(labels[i], xs[i] + z / 2, ys[i] + z / 2);
  }

  // Text de progrés al centre
  char buf[24];
  snprintf(buf, sizeof(buf), "%u / 4", cornerHitCount);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_BG);
  tft.setTextSize(3);
  tft.drawString(buf, SCREEN_W / 2, SCREEN_H / 2 - 10);

  // Dibuixem els 4 indicadors de pas (punts/barres de progrés) per sota del
  // text de progrés
  int dotY = SCREEN_H / 2 + 25;
  int dotW = 20;
  int dotH = 10;
  int gap = 10;
  int totalWidth = (4 * dotW) + (3 * gap);
  int startX = (SCREEN_W - totalWidth) / 2;

  for (int i = 0; i < 4; i++) {
    uint16_t col = (cornerHitCount > i) ? UI_COL_OK : UI_COL_CARD;
    tft.fillRect(startX + i * (dotW + gap), dotY, dotW, dotH, col);
    tft.drawRect(startX + i * (dotW + gap), dotY, dotW, dotH, UI_COL_MUTED);
  }

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

static bool checkCornerTouch(uint16_t tx, uint16_t ty) {
  int z = CORNER_HIT_ZONE_PX;
  struct Zone {
    int x, y;
  };

  // Patró seqüencial: TL → TR → BL → BR
  Zone zones[4] = {
      {0, 0},                      // Pas 0: TL
      {SCREEN_W - z, 0},           // Pas 1: TR
      {0, SCREEN_H - z},           // Pas 2: BL
      {SCREEN_W - z, SCREEN_H - z} // Pas 3: BR
  };
  const char *names[4] = {"TL", "TR", "BL", "BR"};

  uint8_t expected = cornerHitCount;
  if (expected >= CORNER_COUNT)
    return false;

  Zone &z2 = zones[expected];
  // DEBUG: mostra on cau el toc i quina zona s'espera
  Serial.printf("[check] toc px=(%u,%u)  esperat=%s zona x=%d..%d y=%d..%d\n",
                tx, ty, names[expected], z2.x, z2.x + z, z2.y, z2.y + z);

  if (tx >= z2.x && tx < z2.x + z && ty >= z2.y && ty < z2.y + z) {
    if (cornerHitCount == 0)
      firstCornerMs = millis();
    cornerHit[expected] = true;
    cornerHitCount++;
    Serial.printf("[unlock] corner %u (%s) HIT!\n", expected, names[expected]);
    if (cornerHitCount == CORNER_COUNT) {
      Serial.println(F("[unlock] pattern complet!"));
      return true;
    }
  } else {
    Serial.println(F("[unlock] fora de zona"));
  }
  return false;
}

static void resetUnlockPattern() {
  for (int i = 0; i < CORNER_COUNT; i++)
    cornerHit[i] = false;
  cornerHitCount = 0;
  firstCornerMs = 0;
}

static void drawMainMenu() {
  tft.fillRect(0, 0, SCREEN_W, 40, UI_COL_CARD);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("CatFeeder", 10, 20);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(uiRequestedMode == UI_MODE_AUTO ? UI_COL_OK : UI_COL_WARN,
                   UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString(uiRequestedMode == UI_MODE_AUTO ? "Mode: AUTO"
                                                 : "Mode: MANUAL",
                 SCREEN_W / 2, 20);

  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_CARD);
  tft.setTextSize(1);
  char tbuf[12];
  if (telemetry.rtcOk) {
    DateTime now = rtc.now();
    snprintf(tbuf, sizeof(tbuf), "%02u:%02u:%02u", now.hour(), now.minute(),
             now.second());
  } else {
    snprintf(tbuf, sizeof(tbuf), "--:--:--");
  }
  tft.drawString(tbuf, SCREEN_W - 10, 20);

  int bw = 150, bh = 160, by = 45;
  int bx2 = 164, bw2 = 150;
  bool isManual = (uiRequestedMode == UI_MODE_MANUAL);
  uint16_t colManual = isManual ? UI_COL_ACCENT : UI_COL_CARD;

  tft.fillRect(6, by, bw, bh, colManual);
  tft.drawRect(6, by, bw, bh, isManual ? UI_COL_TEXT : UI_COL_MUTED);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(isManual ? UI_COL_BG : UI_COL_TEXT, colManual);
  tft.setTextSize(3);
  tft.drawString("MANUAL", 6 + bw / 2, by + 45);
  tft.setTextSize(1);
  tft.drawString("Dispensar ara", 6 + bw / 2, by + 85);
  tft.drawString("Premer el boto per", 6 + bw / 2, by + 98);
  tft.drawString("controlar el motor.", 6 + bw / 2, by + 111);
  if (isManual) {
    tft.setTextSize(2);
    tft.setTextColor(UI_COL_BG, colManual);
    tft.drawString("ACTIU", 6 + bw / 2, by + 145);
  }

  bool isAuto = (uiRequestedMode == UI_MODE_AUTO);
  uint16_t colAuto = isAuto ? UI_COL_OK : UI_COL_CARD;

  tft.fillRect(bx2, by, bw2, bh, colAuto);
  tft.drawRect(bx2, by, bw2, bh, isAuto ? UI_COL_TEXT : UI_COL_MUTED);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(isAuto ? UI_COL_BG : UI_COL_TEXT, colAuto);
  tft.setTextSize(3);
  tft.drawString("AUTO", bx2 + bw2 / 2, by + 45);
  tft.setTextSize(1);
  tft.drawString("Seguir horaris", bx2 + bw2 / 2, by + 85);
  tft.drawString("configurats a Supabase.", bx2 + bw2 / 2, by + 98);
  if (isAuto) {
    tft.setTextSize(2);
    tft.setTextColor(UI_COL_BG, colAuto);
    tft.drawString("ACTIU", bx2 + bw2 / 2, by + 145);
  }

  tft.fillRect(0, SCREEN_H - 35, SCREEN_W, 35, UI_COL_CARD);
  char fbuf[80];
  snprintf(fbuf, sizeof(fbuf), "Pes: %.1fg T:%.0fC H:%.0f%% %s",
           telemetry.weightG, telemetry.temperatureC, telemetry.humidity,
           telemetry.wifiUp ? "WiFi OK" : "Sense WiFi");
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_MUTED, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString(fbuf, 8, SCREEN_H - 17);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

static void handleMainMenuTouch(uint16_t tx, uint16_t ty) {
  int bw = 150, bh = 160, by = 45;
  int bx2 = 164, bw2 = 150;

  if (tx >= 6 && tx <= 6 + bw && ty >= by && ty <= by + bh) {
    uiRequestedMode = UI_MODE_MANUAL;
    transitionTo(UI_SCREEN_MANUAL_FEED);
    return;
  }
  if (tx >= bx2 && tx <= bx2 + bw2 && ty >= by && ty <= by + bh) {
    uiRequestedMode = UI_MODE_AUTO;
    transitionTo(UI_SCREEN_AUTO_MODE);
    return;
  }
}

static void drawManualFeedScreen() {
  tft.fillRect(0, 0, SCREEN_W, 40, UI_COL_CARD);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("< Enrere", 10, 20);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString("DISPENSACIO MANUAL", SCREEN_W / 2, 20);

  char wbuf[20];
  snprintf(wbuf, sizeof(wbuf), "%.1f g", telemetry.weightG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(5);
  tft.drawString(wbuf, SCREEN_W / 2, 85);
  tft.setTextSize(1);
  tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
  tft.drawString("pes al plat", SCREEN_W / 2, 120);

  if (cycle.active) {
    int bx = 20, bby = 135, bw = SCREEN_W - 40, bbh = 14;
    float frac = (cycle.targetG > 0) ? cycle.dispensedG / cycle.targetG : 0;
    if (frac > 1)
      frac = 1;
    tft.drawRect(bx, bby, bw, bbh, UI_COL_MUTED);
    tft.fillRect(bx + 1, bby + 1, (int)((bw - 2) * frac), bbh - 2,
                 UI_COL_ACCENT);
    char pbuf[24];
    snprintf(pbuf, sizeof(pbuf), "%.1f / %.1f g", cycle.dispensedG,
             cycle.targetG);
    tft.setTextSize(1);
    tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
    tft.drawString(pbuf, SCREEN_W / 2, bby + bbh + 6);
  } else {
    tft.fillRect(20, 135, SCREEN_W - 40, 30, UI_COL_BG);
  }

  int btnY = 162, btnH = 43;
  bool motorRunning = uiManualFeedActive && cycle.active;
  uint16_t btnCol = motorRunning ? UI_COL_DANGER : UI_COL_ACCENT;

  tft.fillRect(20, btnY, SCREEN_W - 40, btnH, btnCol);
  tft.drawRect(20, btnY, SCREEN_W - 40, btnH, UI_COL_TEXT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(motorRunning ? UI_COL_TEXT : UI_COL_BG, btnCol);
  tft.setTextSize(2);
  tft.drawString(motorRunning ? "ATURAR" : "MANTÉ PER DISPENSAR", SCREEN_W / 2,
                 btnY + btnH / 2);

  tft.fillRect(0, SCREEN_H - 30, SCREEN_W, 30, UI_COL_CARD);
  const char *stateStr = "Idle";
  if (currentState == STATE_DISPENSING)
    stateStr = "Dispensant...";
  if (currentState == STATE_ERROR)
    stateStr = "ERROR";
  char ffbuf[48];
  snprintf(ffbuf, sizeof(ffbuf), "Estat: %s    %s", stateStr,
           telemetry.wifiUp ? "WiFi OK" : "Sense WiFi");
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_MUTED, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString(ffbuf, 8, SCREEN_H - 15);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

static void handleManualFeedTouch(uint16_t tx, uint16_t ty) {
  if (tx < 120 && ty < 40) {
    uiManualFeedActive = false;
    transitionTo(UI_SCREEN_MAIN);
    return;
  }
  if (tx >= 20 && tx <= SCREEN_W - 20 && ty >= 162 && ty <= 205) {
    uiManualFeedActive = true;
    screenDirty = true;
  }
}

static void handleManualFeedRelease() {
  if (uiManualFeedActive) {
    uiManualFeedActive = false;
    screenDirty = true;
  }
}

static void drawAutoModeScreen() {
  tft.fillRect(0, 0, SCREEN_W, 40, UI_COL_CARD);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("< Enrere", 10, 20);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString("MODE AUTOMATIC", SCREEN_W / 2, 20);

  tft.fillRect(0, 42, SCREEN_W, 36, UI_COL_OK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_BG, UI_COL_OK);
  tft.setTextSize(2);
  tft.drawString("Mode automatic ACTIU", SCREEN_W / 2, 60);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(1);
  int iy = 82;
  tft.drawString("El dispensador segueix els horaris", 12, iy);
  iy += 13;
  tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
  tft.drawString("configurats a l'aplicacio web de Supabase.", 12, iy);
  iy += 17;

  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(2);
  if (scheduleCount > 0) {
    char nbuf[40];
    snprintf(nbuf, sizeof(nbuf), "Horaris actius: %u", scheduleCount);
    tft.drawString(nbuf, 12, iy);
    iy += 22;
  } else {
    tft.setTextColor(UI_COL_WARN, UI_COL_BG);
    tft.drawString("Sense horaris carregats", 12, iy);
    iy += 22;
    tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
    tft.setTextSize(1);
    tft.drawString("Comprova la connexio WiFi i Supabase.", 12, iy);
  }

  char wbuf[24];
  snprintf(wbuf, sizeof(wbuf), "Pes actual: %.1f g", telemetry.weightG);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_BG);
  tft.setTextSize(2);
  tft.drawString(wbuf, 12, SCREEN_H - 90);

  tft.fillRect(20, SCREEN_H - 65, SCREEN_W - 40, 50, UI_COL_DANGER);
  tft.drawRect(20, SCREEN_H - 65, SCREEN_W - 40, 50, UI_COL_TEXT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_DANGER);
  tft.setTextSize(2);
  tft.drawString("DESACTIVAR MODE AUTO", SCREEN_W / 2, SCREEN_H - 40);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

static void handleAutoModeTouch(uint16_t tx, uint16_t ty) {
  if (tx < 120 && ty < 40) {
    transitionTo(UI_SCREEN_MAIN);
    return;
  }
  if (ty >= SCREEN_H - 65) {
    uiRequestedMode = UI_MODE_MANUAL;
    transitionTo(UI_SCREEN_MAIN);
    return;
  }
}

static bool getTouchPoint(uint16_t &tx, uint16_t &ty) {
  static bool pressingNow =
      false; // histeresi: evita flickering al llevar el dit

  const uint16_t z = tft.getTouchRawZ();

  // Histeresi: puja amb PRESS, baixa amb RELEASE
  if (!pressingNow && z < TOUCH_Z_PRESS)
    return false;
  if (pressingNow && z < TOUCH_Z_RELEASE) {
    pressingNow = false;
    if (activeUIScreen == UI_SCREEN_MANUAL_FEED) {
      handleManualFeedRelease();
    }
    return false;
  }
  pressingNow = true;

  if (USE_NATIVE_CALIBRATION) {
    bool ok = tft.getTouch(&tx, &ty);
    if (ok) {
      Serial.printf("[touch] px=(%u,%u) rawZ=%u\n", tx, ty, z);
    }
    return ok;
  } else {
    uint16_t rx, ry;
    tft.getTouchRaw(&rx, &ry);

    // Intercanvi d'eixos (si cal)
    uint16_t mx = TOUCH_SWAP_XY ? ry : rx;
    uint16_t my = TOUCH_SWAP_XY ? rx : ry;

    // Mapejat lineal raw → píxels, amb suport d'inversió per eix
    tx = (uint16_t)map(constrain((int32_t)mx, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX),
                       TOUCH_X_INV ? TOUCH_RAW_X_MAX : TOUCH_RAW_X_MIN,
                       TOUCH_X_INV ? TOUCH_RAW_X_MIN : TOUCH_RAW_X_MAX, 0,
                       SCREEN_W - 1);

    ty = (uint16_t)map(constrain((int32_t)my, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX),
                       TOUCH_Y_INV ? TOUCH_RAW_Y_MAX : TOUCH_RAW_Y_MIN,
                       TOUCH_Y_INV ? TOUCH_RAW_Y_MIN : TOUCH_RAW_Y_MAX, 0,
                       SCREEN_H - 1);

    // DEBUG: mostra raw i píxels per poder ajustar els MIN/MAX
    Serial.printf("[touch] raw=(%u,%u) px=(%u,%u) rawZ=%u\n", rx, ry, tx, ty, z);
    return true;
  }
}

void setup() {
  Serial.begin(115200);
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

  if (uiRequestedMode == UI_MODE_MANUAL) {
    if (uiManualFeedActive && !cycle.active) {
      startDispense(100.0, "manual", "", "", "");
    }
    if (!uiManualFeedActive && cycle.active) {
      motorEmergencyStop();
    }
  }

  if (cycle.active) {
    cycle.dispensedG += 0.5;
    telemetry.weightG = cycle.dispensedG;
    if (cycle.dispensedG >= cycle.targetG) {
      cycleFinish("completed", cycle.dispensedG);
    }
  }
}