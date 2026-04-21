# 🐱 CatFeeder — Especificació d'Implementació de la UI Tàctil
**Integrated Project II — GR15 [GEMEC-09UV] · Universitat de Vic — UCC**
**Document destinat a: Antigravity / Implementador de codi**
**Versió:** 1.0 · Abril 2026

---

## 0. Context i Abast

Aquest document defineix de forma completa i autocontinguda tots els canvis
necessaris per integrar una interfície d'usuari tàctil (touch UI) a l'ESP32
del CatFeeder. L'implementador rebrà aquest document i els arxius `.ino`
existents i haurà de poder crear o modificar els fitxers de codi sense
necessitat de cap altra font d'informació.

### Stack tecnològic actual (no canvia)
| Component | Llibreria / Tecnologia |
|---|---|
| Microcontrolador | ESP32 WROOM-32 |
| Pantalla | ILI9488 3.5" SPI (320×480 px) |
| Controlador tàctil | XPT2046 (integrat en el mòdul de pantalla) |
| Llibreria display | TFT_eSPI (Bodmer) |
| Motor dispensador | NEMA 17 + DRV8825 via AccelStepper |
| Pesatge | HX711 + cèl·lula de càrrega 1 kg |
| RTC | DS3231 (I²C) |
| Backend | Supabase REST (PostgreSQL) |

### Arxius existents que s'han de modificar o crear
```
Software/arxiusINOproba/programesProba/
├── CatFeeder.ino       ← MODIFICAR (afegir estat UI al state machine)
├── config.h            ← MODIFICAR (afegir constants touch i UI)
├── Display.ino         ← MODIFICAR (integrar touch, eliminar output-only)
├── Motor.ino           ← NO tocar (ja exposa startDispense / motorEmergencyStop)
├── Network.ino         ← NO tocar (ja exposa pollDeviceConfig / pollPendingCommands)
├── Scale.ino           ← NO tocar
├── Inputs.ino          ← MODIFICAR lleugerament (desactivar BTN_FEED en mode auto)
├── RTCManager.ino      ← NO tocar
├── Environment.ino     ← NO tocar
└── TouchUI.ino         ← CREAR (fitxer nou, nucli d'aquest document)
```

---

## 1. Canvi de Pin: TOUCH_CS

### Problema
El pin GPIO 21 estava assignat a `TOUCH_CS` en el TouchTest original, però
**GPIO 21 és `I2C_SDA`** del DS3231 en el firmware base. Usar-lo per al tàctil
causaria conflictes de bus.

### Solució
Assignar `TOUCH_CS` al **GPIO 5**, que apareix lliure en el mapa de pins
actual (`Electronics/pins.md`).

> ⚠️ **Acció física requerida:** Soldar/connectar un cable del pad `T_CS`
> del mòdul de pantalla al GPIO 5 de l'ESP32. Els altres pads tàctils
> (`T_CLK`, `T_DIN`, `T_DO`) ja comparteixen els pins SPI del display
> (GPIO 18, 23, 19 respectivament) i **no requereixen cap connexió addicional**.

### Taula de connexions tàctils definitiva
| Pad mòdul | ESP32 GPIO | Compartit amb |
|---|---|---|
| `T_CS` | **5** | — (nou) |
| `T_CLK` | 18 | TFT_SCLK |
| `T_DIN` | 23 | TFT_MOSI |
| `T_DO` | 19 | TFT_MISO |
| `T_IRQ` | — | No connectar (mode polling) |

---

## 2. Canvis a `TFT_eSPI/User_Setup.h`

Obrir el fitxer `User_Setup.h` de la llibreria TFT_eSPI instal·lada
(`Documents/Arduino/libraries/TFT_eSPI/User_Setup.h`) i assegurar-se que
conté exactament les línies següents (afegir o modificar les que no hi siguin):

```cpp
// --- Driver ---
#define ILI9488_DRIVER

// --- Pins display ---
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   4

// --- Pins tàctil XPT2046 (NOU) ---
#define TOUCH_CS            5
#define SPI_TOUCH_FREQUENCY 2500000

// --- Freqüències ---
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000

// --- Fonts ---
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define SMOOTH_FONT
```

> Si el fitxer té `#define TFT_CS 5`, canviar-lo a `15` per ser consistent
> amb `pins.md`. La inconsistència existeix als arxius originals; el valor
> correcte per al hardware físic és **GPIO 15**.

---

## 3. Canvis a `config.h`

Afegir les constants següents al final del fitxer `config.h`, **just abans
del `#endif`**:

```cpp
// =========================================================================
// TOUCH UI — Constants afegides per la nova interfície tàctil
// =========================================================================

// --- Pin tàctil (ha de coincidir amb User_Setup.h TOUCH_CS) ---
#define TOUCH_CS_PIN            5

// --- Calibració tàctil XPT2046 ---
// Executar l'exemple TFT_eSPI > Generic > Touch_calibrate per obtenir
// els 5 valors reals i substituir els d'aquí.
// Format: { xMin, xMax, yMin, yMax, swapXY }
#define TOUCH_CAL_DATA          { 300, 3600, 300, 3600, 1 }

// --- Mode repòs (screensaver) ---
#define UI_IDLE_TIMEOUT_MS      30000   // 30 s sense interacció → screensaver
#define CAT_BLINK_INTERVAL_MS   3000    // interval entre parpelleigs de la cara

// --- Patró de desbloqueig (4 cantonades) ---
// Mida de la zona de toc per a cada cantonada (en píxels)
#define CORNER_HIT_ZONE_PX      70
// Temps màxim per completar les 4 cantonades (ms). 0 = sense límit.
#define CORNER_TIMEOUT_MS       5000

// --- Orientació i resolució ---
// Pantalla en landscape: amplada=480, alçada=320
#define SCREEN_W                480
#define SCREEN_H                320

// --- Colors UI (RGB565) ---
#define UI_COL_BG               0x0820   // Blau molt fosc (fons global)
#define UI_COL_CARD             0x1082   // Blau fosc per a targetes/botons
#define UI_COL_ACCENT           0x07FF   // Cyan
#define UI_COL_OK               0x07E0   // Verd
#define UI_COL_DANGER           0xF800   // Vermell
#define UI_COL_WARN             0xFD20   // Taronja
#define UI_COL_TEXT             0xFFFF   // Blanc
#define UI_COL_MUTED            0x7BCF   // Gris clar
#define UI_COL_OVERLAY          0x0000   // Negre (screensaver)
#define UI_COL_CAT_LINE         0xC618   // Gris clar per als traços de la cara de gat

// --- Mode de funcionament ---
// (llegit/escrit per TouchUI.ino i consumit per CatFeeder.ino / Network.ino)
// Valors: UI_MODE_MANUAL o UI_MODE_AUTO (definits a TouchUI.ino)
```

---

## 4. Nou fitxer: `TouchUI.ino`

Crear el fitxer `TouchUI.ino` a la mateixa carpeta que `CatFeeder.ino`.
Aquest és el fitxer principal d'aquest document. Copiar el codi complet
que es mostra a continuació.

```cpp
// ==========================================================================
// TouchUI.ino — Interfície d'usuari tàctil per al CatFeeder
// Integrated Project II — GR15 [GEMEC-09UV]
// ==========================================================================
// Estats de la UI (independents del DeviceState del firmware):
//
//   UI_SCREEN_SCREENSAVER  → cara de gat animada, parpelleig d'ulls
//   UI_SCREEN_UNLOCK       → instruccions + feedback de les 4 cantonades
//   UI_SCREEN_MAIN         → menú principal (botons Manual i Auto)
//   UI_SCREEN_MANUAL_FEED  → pantalla de dispensació manual activa
//   UI_SCREEN_AUTO_MODE    → pantalla de mode automàtic actiu
//
// Regla d'or: TouchUI.ino MAI crida directament a funcions de Network.ino.
// La comunicació cap al firmware es fa exclusivament mitjançant les variables
// globals `uiRequestedMode` i `uiManualFeedActive`.
// ==========================================================================

// ---------- Estat de la UI ------------------------------------------------
enum UIScreen {
  UI_SCREEN_SCREENSAVER,
  UI_SCREEN_UNLOCK,
  UI_SCREEN_MAIN,
  UI_SCREEN_MANUAL_FEED,
  UI_SCREEN_AUTO_MODE
};

// Modes de funcionament exportats a CatFeeder.ino
enum UIMode {
  UI_MODE_MANUAL,   // l'usuari controla el dispensador manualment
  UI_MODE_AUTO      // el firmware segueix els horaris de Supabase
};

// ---------- Variables globals (llegides des de CatFeeder.ino) -------------
UIScreen activeUIScreen  = UI_SCREEN_SCREENSAVER;
UIMode   uiRequestedMode = UI_MODE_MANUAL;  // mode per defecte en arrencar
bool     uiManualFeedActive = false;        // true mentre el botó Manual està premut

// ---------- Variables internes de la UI -----------------------------------
static uint32_t lastTouchMs     = 0;   // última interacció vàlida
static uint32_t lastBlinkMs     = 0;   // darrer parpelleig de la cara
static bool     eyesOpen        = true;
static bool     screenDirty     = true; // força redibujat complet

// Patró de les 4 cantonades
// Ordre requerit: TL → TR → BL → BR  (Top-Left, Top-Right, Bot-Left, Bot-Right)
static const uint8_t CORNER_COUNT = 4;
static bool   cornerHit[CORNER_COUNT] = {false, false, false, false};
static uint8_t cornerHitCount = 0;
static uint32_t firstCornerMs = 0;

// Calibració tàctil
static uint16_t touchCalData[5] = TOUCH_CAL_DATA;

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

// ==========================================================================
//                          INICIALITZACIÓ
// ==========================================================================
void touchUIInit() {
  tft.setTouch(touchCalData);
  lastTouchMs = millis();
  transitionTo(UI_SCREEN_SCREENSAVER);
  Serial.println(F("[touchUI] initialised"));
}

// ==========================================================================
//                ACTUALITZACIÓ (cridar des de loop() ~60 ms)
// ==========================================================================
void touchUIUpdate() {
  const uint32_t now = millis();

  // --- Llegir touch --------------------------------------------------------
  uint16_t tx = 0, ty = 0;
  bool touched = getTouchPoint(tx, ty);

  if (touched) lastTouchMs = now;

  // --- Màquina d'estats ----------------------------------------------------
  switch (activeUIScreen) {

    // -----------------------------------------------------------------------
    case UI_SCREEN_SCREENSAVER:
      // Parpelleig d'ulls
      if (now - lastBlinkMs >= CAT_BLINK_INTERVAL_MS) {
        lastBlinkMs = now;
        eyesOpen = !eyesOpen;
        drawCatFace(SCREEN_W / 2, SCREEN_H / 2, !eyesOpen);
      }
      // Qualsevol toc → anar a desbloqueig
      if (touched) {
        transitionTo(UI_SCREEN_UNLOCK);
      }
      break;

    // -----------------------------------------------------------------------
    case UI_SCREEN_UNLOCK:
      if (touched) {
        bool patternComplete = checkCornerTouch(tx, ty);
        // Redibujar sempre que hi hagi toc per mostrar feedback de progrés
        screenDirty = true;
        drawUnlockScreen();
        if (patternComplete) {
          delay(300);  // pausa visual de confirmació
          transitionTo(UI_SCREEN_MAIN);
        }
        // Timeout del patró
        if (cornerHitCount > 0 &&
            CORNER_TIMEOUT_MS > 0 &&
            (now - firstCornerMs) > CORNER_TIMEOUT_MS) {
          resetUnlockPattern();
          screenDirty = true;
        }
      }
      // Timeout de 30 s sense completar → tornar a screensaver
      if (now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
        transitionTo(UI_SCREEN_SCREENSAVER);
      }
      break;

    // -----------------------------------------------------------------------
    case UI_SCREEN_MAIN:
      if (touched) {
        handleMainMenuTouch(tx, ty);
      }
      if (now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
        transitionTo(UI_SCREEN_SCREENSAVER);
      }
      // Redibujar periòdicament per actualitzar rellotge i estat
      if (now % 1000 < 60) screenDirty = true;
      if (screenDirty) drawMainMenu();
      break;

    // -----------------------------------------------------------------------
    case UI_SCREEN_MANUAL_FEED:
      if (touched) {
        handleManualFeedTouch(tx, ty);
        lastTouchMs = now;
      }
      // Redibujar per actualitzar el pes en temps real
      if (now % 250 < 60) screenDirty = true;
      if (screenDirty) drawManualFeedScreen();
      break;

    // -----------------------------------------------------------------------
    case UI_SCREEN_AUTO_MODE:
      if (touched) {
        handleAutoModeTouch(tx, ty);
        lastTouchMs = now;
      }
      if (now - lastTouchMs > UI_IDLE_TIMEOUT_MS) {
        transitionTo(UI_SCREEN_SCREENSAVER);
      }
      if (now % 1000 < 60) screenDirty = true;
      if (screenDirty) drawAutoModeScreen();
      break;
  }
}

// ==========================================================================
//               TRANSICIÓ ENTRE PANTALLES
// ==========================================================================
static void transitionTo(UIScreen next) {
  // Neteja accions pendents en sortir d'una pantalla
  if (activeUIScreen == UI_SCREEN_MANUAL_FEED) {
    uiManualFeedActive = false;  // atura motor si s'abandona la pantalla manual
  }
  activeUIScreen = next;
  screenDirty    = true;
  eyesOpen       = true;
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
//               DIBUIX: SCREENSAVER (cara de gat animada)
// ==========================================================================
// La cara és un cercle amb: dues orelles triangulars, ulls (rectangles
// arrodonits o línies), nas triangular i bigotis (6 línies).
// Quan `blinkNow` és true, els ulls es dibuixen tancats (línia horitzontal).
// ==========================================================================
static void drawCatFace(int cx, int cy, bool blinkNow) {
  const uint16_t C = UI_COL_CAT_LINE;
  const uint16_t B = UI_COL_OVERLAY;  // color de fons per esborrar

  // Cap (cercle)
  tft.drawCircle(cx, cy, 90, C);
  tft.drawCircle(cx, cy, 89, C);  // doble per gruix

  // Orelles (triangles)
  // Esquerra
  tft.drawLine(cx - 90, cy - 30, cx - 55, cy - 95, C);
  tft.drawLine(cx - 55, cy - 95, cx - 20, cy - 65, C);
  tft.drawLine(cx - 20, cy - 65, cx - 90, cy - 30, C);
  // Dreta
  tft.drawLine(cx + 90, cy - 30, cx + 55, cy - 95, C);
  tft.drawLine(cx + 55, cy - 95, cx + 20, cy - 65, C);
  tft.drawLine(cx + 20, cy - 65, cx + 90, cy - 30, C);

  // Nas (petit triangle)
  tft.fillTriangle(cx - 8, cy + 10, cx + 8, cy + 10, cx, cy + 22, C);

  // Bigotis
  // Esquerra (3 línies)
  tft.drawLine(cx - 55, cy + 5,  cx - 15, cy + 8,  C);
  tft.drawLine(cx - 55, cy + 15, cx - 15, cy + 14, C);
  tft.drawLine(cx - 55, cy + 25, cx - 15, cy + 20, C);
  // Dreta (3 línies)
  tft.drawLine(cx + 55, cy + 5,  cx + 15, cy + 8,  C);
  tft.drawLine(cx + 55, cy + 15, cx + 15, cy + 14, C);
  tft.drawLine(cx + 55, cy + 25, cx + 15, cy + 20, C);

  // Ulls (esborrar zona prèvia i redibujar)
  // Zona d'ulls: rectangle de 30x20 centrat en (cx±28, cy-25)
  tft.fillRect(cx - 55, cy - 45, 30, 22, B);
  tft.fillRect(cx + 25, cy - 45, 30, 22, B);

  if (!blinkNow) {
    // Ulls oberts: arc o rectangle arrodonit (TFT_eSPI no té drawRoundRect)
    // Usem tres línies per simular arc
    tft.drawLine(cx - 54, cy - 35, cx - 27, cy - 44, C);  // arc superior E
    tft.drawLine(cx - 54, cy - 35, cx - 27, cy - 27, C);  // arc inferior E
    tft.drawLine(cx + 26, cy - 35, cx + 53, cy - 44, C);  // arc superior D
    tft.drawLine(cx + 26, cy - 35, cx + 53, cy - 27, C);  // arc inferior D
    tft.drawLine(cx - 54, cy - 35, cx - 54, cy - 35, C);
    tft.drawLine(cx + 53, cy - 35, cx + 53, cy - 35, C);
    // Pupil·les (quadrat petit)
    tft.fillRect(cx - 44, cy - 40, 8, 12, C);
    tft.fillRect(cx + 36, cy - 40, 8, 12, C);
  } else {
    // Ulls tancats: línia horitzontal (parpelleig)
    tft.drawLine(cx - 54, cy - 35, cx - 26, cy - 35, C);
    tft.drawLine(cx + 26, cy - 35, cx + 54, cy - 35, C);
  }

  // Text "Toca per desbloquejar" (petit, baix)
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(UI_COL_MUTED, UI_COL_OVERLAY);
  tft.setTextSize(1);
  tft.drawString("Toca per desbloquejar", cx, SCREEN_H - 8);
  tft.setTextDatum(TL_DATUM);
}

static void drawScreensaver() {
  tft.fillScreen(UI_COL_OVERLAY);
  drawCatFace(SCREEN_W / 2, SCREEN_H / 2, false);
  eyesOpen    = true;
  lastBlinkMs = millis();
}

// ==========================================================================
//               DIBUIX: PANTALLA DE DESBLOQUEIG (4 cantonades)
// ==========================================================================
// Les zones de toc de cada cantonada es mostren com a quadrats de
// CORNER_HIT_ZONE_PX × CORNER_HIT_ZONE_PX. Quan es toca una cantonada
// correcta, el quadrat es pinta de verd.
// Ordre obligatori: TL → TR → BL → BR
// ==========================================================================
static void drawUnlockScreen() {
  if (screenDirty) {
    tft.fillScreen(UI_COL_BG);
  }

  // Títol
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(2);
  tft.drawString("Desbloqueig de seguretat", SCREEN_W / 2, 12);
  tft.setTextSize(1);
  tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
  tft.drawString("Toca les 4 cantonades en ordre: TL > TR > BL > BR",
                 SCREEN_W / 2, 38);

  // Les 4 zones de cantonada (TL, TR, BL, BR)
  int z = CORNER_HIT_ZONE_PX;
  // [0]=TL [1]=TR [2]=BL [3]=BR
  int xs[4] = {0,          SCREEN_W - z, 0,          SCREEN_W - z};
  int ys[4] = {0,          0,            SCREEN_H - z, SCREEN_H - z};
  const char* labels[4] = {"TL", "TR", "BL", "BR"};

  for (int i = 0; i < 4; i++) {
    uint16_t col = cornerHit[i] ? UI_COL_OK : UI_COL_CARD;
    tft.fillRect(xs[i], ys[i], z, z, col);
    tft.drawRect(xs[i], ys[i], z, z, UI_COL_MUTED);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(cornerHit[i] ? UI_COL_BG : UI_COL_MUTED, col);
    tft.setTextSize(2);
    tft.drawString(labels[i], xs[i] + z/2, ys[i] + z/2);
  }

  // Progrés numèric al centre
  char buf[24];
  snprintf(buf, sizeof(buf), "%u / 4", cornerHitCount);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_BG);
  tft.setTextSize(3);
  tft.drawString(buf, SCREEN_W / 2, SCREEN_H / 2);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

// ---------- Lògica de detecció de cantonades ------------------------------
// Retorna true si les 4 cantonades han estat tocades en ordre.
static bool checkCornerTouch(uint16_t tx, uint16_t ty) {
  int z = CORNER_HIT_ZONE_PX;
  // Definir zones: TL[0], TR[1], BL[2], BR[3]
  struct Zone { int x, y; };
  Zone zones[4] = {
    {0,           0},
    {SCREEN_W-z,  0},
    {0,           SCREEN_H-z},
    {SCREEN_W-z,  SCREEN_H-z}
  };

  // La propera cantonada que s'espera és la número cornerHitCount
  uint8_t expected = cornerHitCount;
  if (expected >= CORNER_COUNT) return false;

  Zone &z2 = zones[expected];
  if (tx >= z2.x && tx < z2.x + z &&
      ty >= z2.y && ty < z2.y + z) {
    if (cornerHitCount == 0) firstCornerMs = millis();
    cornerHit[expected] = true;
    cornerHitCount++;
    Serial.printf("[unlock] corner %u hit (%s)\n", expected,
                  expected==0?"TL":expected==1?"TR":expected==2?"BL":"BR");
    if (cornerHitCount == CORNER_COUNT) {
      Serial.println(F("[unlock] pattern complete!"));
      return true;
    }
  }
  return false;
}

static void resetUnlockPattern() {
  for (int i = 0; i < CORNER_COUNT; i++) cornerHit[i] = false;
  cornerHitCount = 0;
  firstCornerMs  = 0;
}

// ==========================================================================
//               DIBUIX: MENÚ PRINCIPAL
// ==========================================================================
// Layout (landscape 480×320):
//
//  ┌─────────────────────────────────────────────────────┐
//  │  🐱 CatFeeder         Mode: MANUAL     12:34:56     │  ← header (40px)
//  ├──────────────────────┬──────────────────────────────┤
//  │                      │                              │
//  │    [MANUAL]          │    [AUTOMÀTIC]               │  ← botons (200px)
//  │  Dispensació manual  │  Seguir horaris Supabase     │
//  │                      │                              │
//  ├──────────────────────┴──────────────────────────────┤
//  │  Pes: 0.0 g    Temp: 22.1°C    WiFi: ✓    Idle     │  ← footer (40px)
//  └─────────────────────────────────────────────────────┘
// ==========================================================================
static void drawMainMenu() {
  // --- Header ---
  tft.fillRect(0, 0, SCREEN_W, 40, UI_COL_CARD);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("CatFeeder", 10, 20);

  // Mode actual
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(uiRequestedMode == UI_MODE_AUTO ? UI_COL_OK : UI_COL_WARN,
                   UI_COL_CARD);
  tft.drawString(uiRequestedMode == UI_MODE_AUTO ? "Mode: AUTO" : "Mode: MANUAL",
                 SCREEN_W / 2, 20);

  // Rellotge
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_CARD);
  char tbuf[12];
  if (telemetry.rtcOk) {
    DateTime now = rtc.now();
    snprintf(tbuf, sizeof(tbuf), "%02u:%02u:%02u",
             now.hour(), now.minute(), now.second());
  } else {
    snprintf(tbuf, sizeof(tbuf), "--:--:--");
  }
  tft.drawString(tbuf, SCREEN_W - 10, 20);

  // --- Botó MANUAL (esquerra, 230×230 px a partir de y=45) ---
  int bw = 230, bh = 230, by = 45;
  bool isManual = (uiRequestedMode == UI_MODE_MANUAL);
  uint16_t colManual = isManual ? UI_COL_ACCENT : UI_COL_CARD;

  tft.fillRect(5, by, bw, bh, colManual);
  tft.drawRect(5, by, bw, bh,
               isManual ? UI_COL_TEXT : UI_COL_MUTED);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(isManual ? UI_COL_BG : UI_COL_TEXT, colManual);
  tft.setTextSize(3);
  tft.drawString("MANUAL", 5 + bw/2, by + 60);
  tft.setTextSize(1);
  tft.drawString("Dispensar ara", 5 + bw/2, by + 110);
  tft.drawString("Premer el boto per", 5 + bw/2, by + 126);
  tft.drawString("controlar el motor.", 5 + bw/2, by + 142);
  if (isManual) {
    tft.setTextSize(2);
    tft.setTextColor(UI_COL_BG, colManual);
    tft.drawString("ACTIU", 5 + bw/2, by + 175);
  }

  // --- Botó AUTOMÀTIC (dreta, 235×230 px) ---
  int bx2 = 245;
  int bw2 = SCREEN_W - bx2 - 5;
  bool isAuto = (uiRequestedMode == UI_MODE_AUTO);
  uint16_t colAuto = isAuto ? UI_COL_OK : UI_COL_CARD;

  tft.fillRect(bx2, by, bw2, bh, colAuto);
  tft.drawRect(bx2, by, bw2, bh,
               isAuto ? UI_COL_TEXT : UI_COL_MUTED);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(isAuto ? UI_COL_BG : UI_COL_TEXT, colAuto);
  tft.setTextSize(3);
  tft.drawString("AUTO", bx2 + bw2/2, by + 60);
  tft.setTextSize(1);
  tft.drawString("Seguir horaris", bx2 + bw2/2, by + 110);
  tft.drawString("configurats a Supabase.", bx2 + bw2/2, by + 126);
  if (isAuto) {
    tft.setTextSize(2);
    tft.setTextColor(UI_COL_BG, colAuto);
    tft.drawString("ACTIU", bx2 + bw2/2, by + 175);
  }

  // --- Footer ---
  tft.fillRect(0, SCREEN_H - 35, SCREEN_W, 35, UI_COL_CARD);
  char fbuf[80];
  snprintf(fbuf, sizeof(fbuf), "Pes: %.1fg  T:%.0fC  H:%.0f%%  %s",
           telemetry.weightG,
           isnan(telemetry.temperatureC) ? 0.0f : telemetry.temperatureC,
           isnan(telemetry.humidity)     ? 0.0f : telemetry.humidity,
           telemetry.wifiUp ? "WiFi OK" : "Sense WiFi");
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_MUTED, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString(fbuf, 8, SCREEN_H - 17);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

// --- Gestió de tocs al menú principal ------------------------------------
static void handleMainMenuTouch(uint16_t tx, uint16_t ty) {
  int bw = 230, bh = 230, by = 45;
  int bx2 = 245, bw2 = SCREEN_W - bx2 - 5;

  // Botó MANUAL (zona: x=5..235, y=45..275)
  if (tx >= 5 && tx <= 5 + bw && ty >= by && ty <= by + bh) {
    uiRequestedMode = UI_MODE_MANUAL;
    transitionTo(UI_SCREEN_MANUAL_FEED);
    return;
  }

  // Botó AUTO (zona: x=245..475, y=45..275)
  if (tx >= bx2 && tx <= bx2 + bw2 && ty >= by && ty <= by + bh) {
    uiRequestedMode = UI_MODE_AUTO;
    transitionTo(UI_SCREEN_AUTO_MODE);
    return;
  }
}

// ==========================================================================
//               DIBUIX: PANTALLA DISPENSACIÓ MANUAL
// ==========================================================================
// Layout:
//  ┌──────────────────────────────────────────────────┐
//  │  ← Enrere           DISPENSACIÓ MANUAL           │ header
//  ├──────────────────────────────────────────────────┤
//  │                                                  │
//  │         Pes actual: 125.3 g                      │ pes gran
//  │                                                  │
//  │  ┌────────────────────────────────────────────┐  │
//  │  │    ████████████░░░░░░░░░░░  125/200 g     │  │ barra progrés
//  │  └────────────────────────────────────────────┘  │
//  │                                                  │
//  │  ┌─────────────────────────────────────────────┐ │
//  │  │         MANTENIR PREMUT PER DISPENSAR       │ │ ← botó polsador
//  │  └─────────────────────────────────────────────┘ │
//  ├──────────────────────────────────────────────────┤
//  │  Estat: Idle    WiFi: OK                         │ footer
//  └──────────────────────────────────────────────────┘
//
// IMPORTANT: El botó de dispensació és un polsador continu.
//   - Mentre l'usuari manté el toc → uiManualFeedActive = true
//   - Quan deixa de tocar     → uiManualFeedActive = false
// CatFeeder.ino és responsable d'engegar/aturar el motor en funció d'aquesta
// variable (veure secció 6 d'aquest document).
// ==========================================================================
static void drawManualFeedScreen() {
  // Header
  tft.fillRect(0, 0, SCREEN_W, 40, UI_COL_CARD);
  // Botó "← Enrere"
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("< Enrere", 10, 20);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_CARD);
  tft.drawString("DISPENSACIO MANUAL", SCREEN_W/2, 20);

  // Pes actual
  char wbuf[20];
  snprintf(wbuf, sizeof(wbuf), "%.1f g", telemetry.weightG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(6);
  tft.drawString(wbuf, SCREEN_W/2, 110);
  tft.setTextSize(1);
  tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
  tft.drawString("pes al plat", SCREEN_W/2, 150);

  // Barra de progrés (si hi ha cicle actiu)
  if (cycle.active) {
    int bx = 20, bby = 165, bw = SCREEN_W - 40, bbh = 18;
    float frac = (cycle.targetG > 0) ? cycle.dispensedG / cycle.targetG : 0;
    if (frac > 1) frac = 1;
    tft.drawRect(bx, bby, bw, bbh, UI_COL_MUTED);
    tft.fillRect(bx+1, bby+1, (int)((bw-2)*frac), bbh-2, UI_COL_ACCENT);
    char pbuf[24];
    snprintf(pbuf, sizeof(pbuf), "%.1f / %.1f g",
             cycle.dispensedG, cycle.targetG);
    tft.setTextSize(1);
    tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
    tft.drawString(pbuf, SCREEN_W/2, bby + bbh + 6);
  } else {
    tft.fillRect(20, 160, SCREEN_W - 40, 30, UI_COL_BG);
  }

  // Botó polsador "MANTENIR PREMUT"
  int btnY = 200, btnH = 70;
  bool motorRunning = uiManualFeedActive && cycle.active;
  uint16_t btnCol = motorRunning ? UI_COL_DANGER : UI_COL_ACCENT;

  tft.fillRect(20, btnY, SCREEN_W - 40, btnH, btnCol);
  tft.drawRect(20, btnY, SCREEN_W - 40, btnH, UI_COL_TEXT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(motorRunning ? UI_COL_TEXT : UI_COL_BG, btnCol);
  tft.setTextSize(2);
  tft.drawString(motorRunning ? "ATURAR" : "MANTENIR PREMUT PER DISPENSAR",
                 SCREEN_W/2, btnY + btnH/2);

  // Footer
  tft.fillRect(0, SCREEN_H - 30, SCREEN_W, 30, UI_COL_CARD);
  const char* stateStr = "Idle";
  if (currentState == STATE_DISPENSING) stateStr = "Dispensant...";
  if (currentState == STATE_ERROR)      stateStr = "ERROR";
  char ffbuf[48];
  snprintf(ffbuf, sizeof(ffbuf), "Estat: %s    %s",
           stateStr, telemetry.wifiUp ? "WiFi OK" : "Sense WiFi");
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_MUTED, UI_COL_CARD);
  tft.setTextSize(1);
  tft.drawString(ffbuf, 8, SCREEN_H - 15);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

// --- Gestió de tocs a la pantalla manual ----------------------------------
static void handleManualFeedTouch(uint16_t tx, uint16_t ty) {
  // Botó "← Enrere" (header, x=0..120, y=0..40)
  if (tx < 120 && ty < 40) {
    uiManualFeedActive = false;
    transitionTo(UI_SCREEN_MAIN);
    return;
  }
  // Botó DISPENSAR (x=20..460, y=200..270)
  if (tx >= 20 && tx <= SCREEN_W-20 && ty >= 200 && ty <= 270) {
    // Mentre hi ha toc en aquesta zona → activar motor
    uiManualFeedActive = true;
    screenDirty = true;
  }
}

// Cridat quan NO hi ha toc (touch released). Atura el motor.
static void handleManualFeedRelease() {
  if (uiManualFeedActive) {
    uiManualFeedActive = false;
    screenDirty = true;
  }
}

// ==========================================================================
//               DIBUIX: PANTALLA MODE AUTOMÀTIC
// ==========================================================================
// Layout:
//  ┌──────────────────────────────────────────────────┐
//  │  ← Enrere              MODE AUTOMÀTIC            │ header
//  ├──────────────────────────────────────────────────┤
//  │  ✅ Mode automàtic ACTIU                         │
//  │  El dispensador segueix els horaris de Supabase  │
//  │                                                  │
//  │  Proper àpat: 08:30 (25g)                        │ info horari
//  │  Últim àpat:  23:01 (24.8g) — completat          │
//  │  Pes actual:  0.0g                               │
//  │                                                  │
//  │  ┌─────────────────────────────────────────────┐ │
//  │  │         DESACTIVAR MODE AUTO                │ │ botó sortida
//  │  └─────────────────────────────────────────────┘ │
//  └──────────────────────────────────────────────────┘
// ==========================================================================
static void drawAutoModeScreen() {
  // Header
  tft.fillRect(0, 0, SCREEN_W, 40, UI_COL_CARD);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_CARD);
  tft.setTextSize(2);
  tft.drawString("< Enrere", 10, 20);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_CARD);
  tft.drawString("MODE AUTOMATIC", SCREEN_W/2, 20);

  // Banner estat
  tft.fillRect(0, 45, SCREEN_W, 50, UI_COL_OK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_BG, UI_COL_OK);
  tft.setTextSize(2);
  tft.drawString("Mode automatic ACTIU", SCREEN_W/2, 70);

  // Info horaris (provinent de `schedules[]` de RTCManager.ino)
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(2);
  int iy = 110;
  tft.drawString("El dispensador segueix els horaris", 12, iy); iy += 24;
  tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
  tft.setTextSize(1);
  tft.drawString("configurats a l'aplicacio web de Supabase.", 12, iy); iy += 20;

  // Proper àpat (llegir de la taula `schedules` global si és accessible)
  tft.setTextColor(UI_COL_TEXT, UI_COL_BG);
  tft.setTextSize(2);
  if (scheduleCount > 0) {
    char nbuf[40];
    snprintf(nbuf, sizeof(nbuf), "Horaris actius: %u", scheduleCount);
    tft.drawString(nbuf, 12, iy); iy += 28;
  } else {
    tft.setTextColor(UI_COL_WARN, UI_COL_BG);
    tft.drawString("Sense horaris carregats", 12, iy); iy += 28;
    tft.setTextColor(UI_COL_MUTED, UI_COL_BG);
    tft.setTextSize(1);
    tft.drawString("Comprova la connexio WiFi i la configuracio a Supabase.", 12, iy);
  }

  // Pes actual
  char wbuf[24];
  snprintf(wbuf, sizeof(wbuf), "Pes actual: %.1f g", telemetry.weightG);
  tft.setTextColor(UI_COL_ACCENT, UI_COL_BG);
  tft.setTextSize(2);
  tft.drawString(wbuf, 12, SCREEN_H - 90);

  // Botó "DESACTIVAR MODE AUTO"
  tft.fillRect(20, SCREEN_H - 65, SCREEN_W - 40, 50, UI_COL_DANGER);
  tft.drawRect(20, SCREEN_H - 65, SCREEN_W - 40, 50, UI_COL_TEXT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_COL_TEXT, UI_COL_DANGER);
  tft.setTextSize(2);
  tft.drawString("DESACTIVAR MODE AUTO", SCREEN_W/2, SCREEN_H - 40);

  tft.setTextDatum(TL_DATUM);
  screenDirty = false;
}

// --- Gestió de tocs a la pantalla auto ------------------------------------
static void handleAutoModeTouch(uint16_t tx, uint16_t ty) {
  // Botó "← Enrere" (header)
  if (tx < 120 && ty < 40) {
    transitionTo(UI_SCREEN_MAIN);
    return;
  }
  // Botó "DESACTIVAR" (baix)
  if (ty >= SCREEN_H - 65) {
    uiRequestedMode = UI_MODE_MANUAL;
    transitionTo(UI_SCREEN_MAIN);
    return;
  }
}

// ==========================================================================
//               LECTURA DEL TÀCTIL
// ==========================================================================
// Retorna true si hi ha un toc vàlid i escriu les coordenades a tx, ty.
// Gestiona automàticament el "release" del botó de dispensació manual.
// ==========================================================================
static bool getTouchPoint(uint16_t &tx, uint16_t &ty) {
  const uint16_t z = tft.getTouchRawZ();
  if (z < 400) {
    // No hi ha toc → gestionar release si estàvem dispensant manualment
    if (activeUIScreen == UI_SCREEN_MANUAL_FEED) {
      handleManualFeedRelease();
    }
    return false;
  }
  bool ok = tft.getTouch(&tx, &ty);
  return ok;
}
```

---

## 5. Modificacions a `CatFeeder.ino`

### 5.1 Afegir declaració de la variable de mode al bloc de globals

Afegir just **sota** la definició de `activeScreen`:

```cpp
// Mode de la UI (definit a TouchUI.ino, llegit aquí per controlar lògica)
extern UIMode   uiRequestedMode;
extern bool     uiManualFeedActive;
extern UIScreen activeUIScreen;
```

### 5.2 Modificar `setup()` — afegir `touchUIInit()`

Dins de `setup()`, just **després** de `displayInit()`:

```cpp
// --- Inicialitzar UI tàctil (AFEGIR) ---
touchUIInit();
```

Substituir `displaySplash("Ready")` per la transició a screensaver:
```cpp
// Eliminar: displaySplash("Ready");
// Afegir:
// (touchUIInit ja ha fet transitionTo(UI_SCREEN_SCREENSAVER))
```

### 5.3 Modificar `loop()` — integrar UI tàctil i lògica de mode

Substituir el bloc del **step 5 (Display refresh)** per:

```cpp
// 5) UI tàctil (~60 ms)
if (now - tLastDisplay >= 60) {
  tLastDisplay = now;
  touchUIUpdate();   // <-- substitueix displayUpdate()
}
```

Modificar el **step 3 (State machine)** per respectar el mode seleccionat:

```cpp
// 3) State machine
switch (currentState) {
  case STATE_IDLE:
    // Modo AUTO: seguir schedules Supabase (comportament original)
    if (uiRequestedMode == UI_MODE_AUTO) {
      checkScheduledFeeds();
    }
    // Modo MANUAL: engegar motor mentre l'usuari prem el botó de la UI
    if (uiRequestedMode == UI_MODE_MANUAL && uiManualFeedActive) {
      if (!cycle.active) {
        // Dispensació sense límit de pes (maxim config per seguretat)
        startDispense(MAX_PORTION_G, "manual", String(""), String(""), String(""));
      }
    }
    break;

  case STATE_DISPENSING:
    runDispensingCycle();
    // Si el mode és MANUAL i l'usuari ha deixat anar → aturar motor
    if (uiRequestedMode == UI_MODE_MANUAL && !uiManualFeedActive && cycle.active) {
      motorEmergencyStop();
      cycleFinish("completed", cycle.dispensedG);
    }
    break;

  case STATE_ERROR:
    // Redirigir a la UI d'error si no hi som ja
    break;

  default:
    break;
}
```

### 5.4 Modificar `handleButtonEvents()` a `Inputs.ino`

Al botó `BTN_FEED`, afegir comprovació del mode:

```cpp
if (in_feed.pressedEdge) {
  in_feed.pressedEdge = false;
  // En mode AUTO, el botó físic segueix funcionant (redundància de seguretat)
  // En mode MANUAL, la UI tàctil és el control principal, però el botó físic
  // també funciona com a polsador de dispensació ràpida.
  if (currentState == STATE_IDLE) {
    Serial.println(F("[btn] FEED pressed"));
    startDispense(DEFAULT_PORTION_G, "manual", String(""), String(""), String(""));
  } else {
    Serial.println(F("[btn] FEED ignored (not idle)"));
  }
}
```

---

## 6. Integració Motor ↔ TouchUI (diagrama de flux)

```
Usuario toca botó MANUAL a UI_SCREEN_MAIN
         │
         ▼
  transitionTo(UI_SCREEN_MANUAL_FEED)
  uiRequestedMode = UI_MODE_MANUAL
         │
         ▼
  Usuari MANTÉ PREMUT el botó "DISPENSAR"
         │
  getTouchPoint → zona botó → uiManualFeedActive = true
         │
         ▼
  CatFeeder.ino loop():
    STATE_IDLE + uiManualFeedActive == true
         │
         └──► startDispense(MAX_PORTION_G, "manual", ...)
              currentState = STATE_DISPENSING
              Motor engegat (AccelStepper)
         │
  Cada 250ms: drawManualFeedScreen() actualitza barra pes
         │
  Usuari DEIXA de prémer
         │
  getTouchPoint retorna false → handleManualFeedRelease()
  uiManualFeedActive = false
         │
         ▼
  CatFeeder.ino loop():
    STATE_DISPENSING + uiManualFeedActive == false
         │
         └──► motorEmergencyStop()
              cycleFinish("completed", dispensedG)
              logFeedEvent → Supabase
              currentState = STATE_IDLE
```

---

## 7. Integració Mode Automàtic ↔ Supabase

Quan `uiRequestedMode == UI_MODE_AUTO`:

1. `checkScheduledFeeds()` (a `RTCManager.ino`) és cridada des del `loop()` normalment.
2. `pollDeviceConfig()` (a `Network.ino`) refresca els horaris cada 60 s.
3. La UI `UI_SCREEN_AUTO_MODE` llegeix `scheduleCount` (variable global de `RTCManager.ino`) per mostrar quants horaris hi ha actius.
4. **No cal cap canvi a `Network.ino` ni a `RTCManager.ino`.**

> La variable `uiRequestedMode` actua com a interruptor: quan val `UI_MODE_MANUAL`,
> `checkScheduledFeeds()` **no es crida** i el firmware ignora els horaris de Supabase.

---

## 8. Calibració del Tàctil (pas obligatori)

Abans de desplegar a producció, executar la calibració real:

1. Flash de l'exemple: `Arxiu → Exemples → TFT_eSPI → Generic → Touch_calibrate`
2. Seguir les instruccions en pantalla (tocar les 4 cantonades quan es demana).
3. El Serial Monitor imprimirà quelcom com:
   ```
   uint16_t calData[5] = { 281, 3535, 347, 3583, 7 };
   ```
4. Substituir el valor de `TOUCH_CAL_DATA` a `config.h`:
   ```cpp
   #define TOUCH_CAL_DATA  { 281, 3535, 347, 3583, 7 }
   ```
5. Re-compilar i flashejar el firmware complet.

---

## 9. Resum de Canvis per Fitxer

| Fitxer | Acció | Detall |
|---|---|---|
| `TFT_eSPI/User_Setup.h` | **Modificar** | Afegir `TOUCH_CS 5` i `SPI_TOUCH_FREQUENCY` |
| `config.h` | **Modificar** | Afegir bloc de constants `TOUCH UI` (secció 3) |
| `TouchUI.ino` | **Crear** | Fitxer nou, codi complet a secció 4 |
| `CatFeeder.ino` | **Modificar** | Integrar `touchUIUpdate()`, lògica de mode (secció 5) |
| `Inputs.ino` | **Modificar lleugerament** | Comprovació de mode al BTN_FEED (secció 5.4) |
| `Display.ino` | **Mantenir tal com és** | La UI tàctil NO crida `displayUpdate()` |
| `Motor.ino` | **No tocar** | `startDispense()` i `motorEmergencyStop()` ja existents |
| `Network.ino` | **No tocar** | `pollDeviceConfig()` ja existent |
| `RTCManager.ino` | **No tocar** | `checkScheduledFeeds()` i `scheduleCount` ja existents |
| `Scale.ino` | **No tocar** | |
| `Environment.ino` | **No tocar** | |

---

## 10. Flux Complet d'Usuari (diagrama)

```
[Power ON]
     │
     ▼
[SCREENSAVER]  ◄──────────────────────────────────────────────┐
  Cara gat animada                                             │
  Parpelleig ulls (3s)                                         │ 30s sense toc
     │                                                         │
  Qualsevol toc                                                │
     │                                                         │
     ▼                                                         │
[DESBLOQUEIG]                                                  │
  Instruccions visibles                                        │
  Tocar: TL → TR → BL → BR  (dins 5s)                         │
     │                                                         │
  Patró correcte                                               │
     │                                                         │
     ▼                                                         │
[MENÚ PRINCIPAL] ◄──────────────────────────────────────────┐ │
  Botó MANUAL  |  Botó AUTO                                  │ │
     │                    │                                  │ │
     ▼                    ▼                                  │ │
[MANUAL FEED]       [AUTO MODE]                              │ │
  Pes en temps real   Horaris actius                         │ │
  Botó polsador       Botó desactivar                        │ │
  ← Enrere            ← Enrere  ──────────────────────────► ┘ │
     │                                                         │
  30s sense toc ───────────────────────────────────────────► ──┘
```

---

## 11. Variables Globals Exportades per TouchUI.ino

Aquestes variables han de ser **accessibles des de `CatFeeder.ino`**. Com que
tot és a la mateixa carpeta Arduino, les variables globals de `TouchUI.ino`
ja són visibles (C++ linkage). Si l'IDE es queixa de redeclaració, afegir
`extern` a `CatFeeder.ino` tal com es mostra a la secció 5.1.

| Variable | Tipus | Propòsit |
|---|---|---|
| `activeUIScreen` | `UIScreen` | Pantalla activa a la UI |
| `uiRequestedMode` | `UIMode` | `UI_MODE_MANUAL` o `UI_MODE_AUTO` |
| `uiManualFeedActive` | `bool` | `true` mentre l'usuari prem el botó de dispensació manual |

---

## 12. Notes per a l'Implementador (Antigravity)

1. **No canviar la lògica de `Motor.ino`, `Scale.ino`, `Network.ino` ni `RTCManager.ino`.** Totes les integracions es fan únicament a través de les variables globals i les funcions ja existents.

2. **`displayUpdate()` queda obsoleta** amb aquesta implementació; `touchUIUpdate()` la substitueix completament. No cridar ambdues.

3. **Mode manual continu:** El botó de dispensació a la pantalla manual funciona com un polsador: motor actiu mentre el dit és a la pantalla, s'atura quan s'aixeca. Això difereix del flux original de `startDispense()` (que s'executa fins al pes objectiu). Per implementar-ho, `startDispense()` s'invoca amb `MAX_PORTION_G` com a objectiu i `motorEmergencyStop()` s'invoca quan `uiManualFeedActive` passa a `false`. L'HX711 continua registrant el pes real dispensat per al log.

4. **Calibració de tàctil:** Els valors per defecte de `TOUCH_CAL_DATA` (`{ 300, 3600, 300, 3600, 1 }`) són genèrics. **És obligatori fer la calibració real** (secció 8) per garantir que les zones de toc (especialment les cantonades del patró de desbloqueig) s'activin on es mostren gràficament.

5. **GPIO 5 per a TOUCH_CS:** Comprovar que en el hardware físic el pad `T_CS` del mòdul de pantalla estigui connectat al GPIO 5. Si no és possible per restriccions de cablejat, usar qualsevol GPIO lliure (p.ex. GPIO 0 amb precaució de strapping) i actualitzar tant `User_Setup.h` com `config.h`.

6. **Compilació:** Tots els fitxers `.ino` de la mateixa carpeta es compilen junts. `TouchUI.ino` utilitza `tft`, `telemetry`, `cycle`, `currentState`, `scheduleCount`, `rtc` i les funcions `motorEmergencyStop()`, `cycleFinish()`, `startDispense()` que estan definides als altres fitxers.

---

*Document generat per al projecte CatFeeder — Integrated Project II GR15 [GEMEC-09UV]*
*Universitat de Vic — Universitat Central de Catalunya — Curs 2025–2026*