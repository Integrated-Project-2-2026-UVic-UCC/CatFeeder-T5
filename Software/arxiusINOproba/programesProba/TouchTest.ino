// ==========================================================================
// TouchTest.ino — Standalone XPT2046 touch-panel tester
// Integrated Project II — GR15 [GEMEC-09UV]
// ==========================================================================
// Hardware : ESP32 WROOM-32 + 3.5" ILI9488 SPI display with XPT2046 touch.
// Goal     : Verify that the touch controller reacts and print each event
//            to the Serial monitor. The display itself is only used to show
//            a static prompt.
// Libraries: TFT_eSPI (Bodmer) — built-in XPT2046 support is used.
//
// Before flashing:
//   1) In TFT_eSPI's `User_Setup.h`, configure the ILI9488 as usual and add:
//        #define TOUCH_CS 21              // free GPIO, wire to T_CS pad
//        #define SPI_TOUCH_FREQUENCY 2500000
//   2) Wire the touch pads of the module:
//        T_CS  -> GPIO21   (matches TOUCH_CS above)
//        T_CLK -> GPIO18   (shared with TFT_SCLK)
//        T_DIN -> GPIO23   (shared with TFT_MOSI)
//        T_DO  -> GPIO19   (shared with TFT_MISO)
//        T_IRQ -> leave disconnected (polled mode)
//   3) Open the Serial monitor at 115200 baud and touch the screen.
// ==========================================================================

#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// Pressure threshold (raw Z from the XPT2046). Anything above this is
// treated as a real touch; below is considered noise / idle.
#define Z_THRESHOLD        400

// Rate-limits to keep the Serial monitor readable.
#define TOUCH_PRINT_MS      40
#define IDLE_PRINT_MS     2000

// Rough default calibration so tft.getTouch() returns something meaningful
// out-of-the-box. For accurate pixel coordinates run the official TFT_eSPI
// example `Touch_calibrate` and paste the 5 values here.
//   { xMin, xMax, yMin, yMax, rotationFlag }
static uint16_t calData[5] = { 300, 3600, 300, 3600, 1 };

static uint32_t tLastTouch = 0;
static uint32_t tLastIdle  = 0;
static uint32_t touchCount = 0;

// --------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("  ILI9488 / XPT2046 — touch panel test"));
  Serial.println(F("========================================"));
  Serial.println(F("Touch the screen. Events print below."));
  Serial.printf ("Z threshold : %u\n", Z_THRESHOLD);
  Serial.println();

  tft.init();
  tft.setRotation(1);                 // landscape 480x320
  tft.fillScreen(TFT_BLACK);

  // On-screen prompt (purely visual).
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawString("Touch test", tft.width() / 2, tft.height() / 2 - 40);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Toca la pantalla", tft.width() / 2, tft.height() / 2 + 5);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Serial @ 115200 baud", tft.width() / 2, tft.height() - 16);

  // Feed the default calibration so getTouch() can map to screen pixels.
  tft.setTouch(calData);
}

// --------------------------------------------------------------------------
void loop() {
  const uint32_t now = millis();

  // Raw pressure reading. When nothing is touched the XPT2046 returns a
  // small value (often < 100). Z grows with firmer presses.
  const uint16_t z = tft.getTouchRawZ();

  if (z > Z_THRESHOLD) {
    // --- Valid touch: read both raw and calibrated coordinates. ----------
    uint16_t rx = 0, ry = 0;
    tft.getTouchRaw(&rx, &ry);

    uint16_t cx = 0, cy = 0;
    bool     haveCal = tft.getTouch(&cx, &cy);

    if (now - tLastTouch >= TOUCH_PRINT_MS) {
      tLastTouch = now;
      touchCount++;
      Serial.printf("#%05lu  TOUCH  raw=(%4u,%4u)  Z=%4u",
                    touchCount, rx, ry, z);
      if (haveCal) {
        Serial.printf("  px=(%3u,%3u)", cx, cy);
      } else {
        Serial.print(F("  px=(no-cal)"));
      }
      Serial.println();
    }
  } else {
    // --- Idle: one heartbeat line every 2 s so you know the sketch is OK. -
    if (now - tLastIdle >= IDLE_PRINT_MS) {
      tLastIdle = now;
      Serial.printf("idle   baseline Z=%u\n", z);
    }
  }
}
