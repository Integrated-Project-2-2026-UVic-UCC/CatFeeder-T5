#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

// ==========================================================================
// CatFeeder — Global Configuration
// Integrated Project II — GR15 [GEMEC-09UV]
// ==========================================================================
// Pin map matches the REAL soldered hardware (see project doc T5).
// ESP32 WROOM-32 + ILI9341 2.8" 320x240 (XPT2046 touch, shared SPI bus)
// HX711 load cell, DHT22, NEMA17 + DRV8825.
// DS3231 RTC removed to avoid I2C conflicts with the touch controller.
// NO physical buttons — the only user interface is the touch screen.
// ==========================================================================

// --------------------------- Firmware meta ---------------------------------
#define FW_VERSION "0.2.0-touch"
#define SERIAL_BAUD 115200

// --------------------------- WiFi ------------------------------------------
#define WIFI_SSID "Lab-Modul"
#define WIFI_PASSWORD "GVe836Nf"
#define WIFI_RECONNECT_MS 10000
#define WIFI_CONNECT_TIMEOUT_MS 15000

// --------------------------- Supabase (REST) -------------------------------
#define SUPABASE_URL "https://jawqxuzlvvzsrobftupx.supabase.co"
#define SUPABASE_ANON_KEY                                                      \
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."                                      \
  "eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imphd3F4dXpsdnZ6c3JvYmZ0dXB4Iiwicm9sZSI6Im" \
  "Fub24iLCJpYXQiOjE3NzM4NjY2MjcsImV4cCI6MjA4OTQ0MjYyN30."                     \
  "FnvhOpqhZh9Z3j2XkIPla1wUbx3wAsaP4anr44Utrzs"
#define DEVICE_ID "44b0f051-549a-4b87-b67a-592254e5c84f"

// --------------------------- Display (ILI9341 320x240, touch XPT2046) ------
// Concrete pins live in User_Setup.h (TFT_eSPI). Touch shares the SPI bus.
//   MOSI = 23  MISO = 19  SCLK = 18  TFT_CS = 5  DC = 22  RST = 4
//   TOUCH_CS = 21        (no backlight pin — BL wired to VCC)
// Rotation 1 = landscape 320x240.
#define DISPLAY_ROTATION 1
#define TOUCH_CS 21

// --------------------------- I2C (shared bus for future peripherals) -------
// DS3231 RTC removed. I2C pins kept in case other I2C devices are added.
#define I2C_SDA 32
#define I2C_SCL 33
#define I2C_CLOCK_HZ 400000

// --------------------------- HX711 + Load Cell -----------------------------
#define HX711_DT 16
#define HX711_SCK 17
#define HX711_GAIN 128
#define HX711_SAMPLES 5
// Obtained via calibration routine (see Scale.ino -> scaleCalibrate()).
#define HX711_CALIBRATION_FACTOR -2027.0f
#define SCALE_STABLE_THRESHOLD_G 0.3f

// --------------------------- DHT22 -----------------------------------------
#define DHT_PIN 14
#define DHT_READ_INTERVAL_MS 2500 // DHT22 minimum sampling is ~2s

// --------------------------- Stepper NEMA 17 + DRV8825 ---------------------
#define STEPPER_STEP 26
#define STEPPER_DIR 27
#define STEPPER_EN 25                  // Active-LOW enable
#define STEPPER_STEPS_PER_REV 200      // 1.8°/step motor
#define STEPPER_MICROSTEPS 16          // Wire MS1/MS2/MS3 accordingly
#define STEPPER_MAX_SPEED 2000.0f      // steps/s (after microstepping)
#define STEPPER_ACCEL 1500.0f          // steps/s^2
#define STEPPER_DISPENSE_SPEED 1200.0f // nominal feed speed
#define STEPPER_MAX_RUN_MS 30000       // hard safety timeout per cycle

// Nominal grams per second at STEPPER_DISPENSE_SPEED. The control loop is
// gravimetric (reads the scale), so this is only used to predict timing.
#define STEPPER_FEED_RATE_GPS 8.0f

// --------------------------- Feeder defaults -------------------------------
#define DEFAULT_PORTION_G 25.0f
#define MIN_PORTION_G 5.0f
#define MAX_PORTION_G 500.0f
#define FEED_TOLERANCE_G 1.5f

// Gravimetric stop: the auger stops when the scale reads this fraction of the
// target. The food still in flight in the channel completes the 100%, which
// avoids over-dispensing.
#define FEED_STOP_THRESHOLD 0.90f
// Non-blocking settling time after the auger stops, to let in-flight food land
// before reading the final dispensed weight.
#define FEED_SETTLE_MS 500

// --------------------------- Timings ---------------------------------------
#define TELEMETRY_INTERVAL_MS 2000
#define HEARTBEAT_INTERVAL_MS 30000
#define COMMAND_POLL_INTERVAL_MS 5000
#define CONFIG_POLL_INTERVAL_MS 60000
#define REALTIME_WEIGHT_INTERVAL_MS 500
#define DISPLAY_REFRESH_MS 250

// --------------------------- Device state & structures ---------------------
enum DeviceState {
  STATE_BOOT,
  STATE_IDLE,
  STATE_DISPENSING,
  STATE_ERROR,
  STATE_OFFLINE_DEGRADED
};
extern DeviceState currentState;
extern const char *lastErrorMessage;

struct Telemetry {
  float weightG; // current net weight on the tray
  float temperatureC;
  float humidity;
  bool wifiUp;
};
extern Telemetry telemetry;

struct FeedingCycle {
  bool active;
  float targetG;
  float dispensedG;
  float baselineG; // tray weight at the moment the cycle starts
  uint32_t startMs;
  uint32_t lastPublishMs;
  const char *trigger;   // "manual" or "scheduled"
  String commandId;      // Supabase commands.id (empty for scheduled)
  String catId;          // cat UUID
  String scheduleId;     // schedule UUID (empty for manual)
  String startedAtIso;   // ISO-8601 timestamp captured at cycle start
};
extern FeedingCycle cycle;

// --------------------------- Touch UI mode & screens -----------------------
enum UIMode {
  UI_MODE_MANUAL,
  UI_MODE_AUTO
};
extern UIMode uiRequestedMode;

enum UIScreen {
  UI_SCREEN_SCREENSAVER,
  UI_SCREEN_UNLOCK,
  UI_SCREEN_MAIN,
  UI_SCREEN_MANUAL_FEED,
  UI_SCREEN_AUTO_MODE
};
extern UIScreen activeUIScreen;
extern bool screenDirty;
extern uint8_t scheduleCount;

// Status & settings for drawing
extern bool eyesOpen;
extern bool cornerHit[4];
extern uint8_t cornerHitCount;

extern uint32_t lastTouchMs;
extern uint32_t lastBlinkMs;
extern uint32_t lastPeriodicRedrawMs;
extern uint32_t lastManualDynamicMs;
extern uint32_t firstCornerMs;

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
#define TOUCH_Z_THRESHOLD       350

#define MAIN_REDRAW_MS          1000
#define AUTO_REDRAW_MS          1000
#define MANUAL_DYNAMIC_MS       250

// ---------------- MANUAL SCREEN LAYOUT ----------------
#define MANUAL_BTN_X            18
#define MANUAL_BTN_Y            146
#define MANUAL_BTN_W            284
#define MANUAL_BTN_H            74

#define MANUAL_BACK_W           96
#define HEADER_H                32
#define FOOTER_H                18

// --------------------------- Global functions ------------------------------
void touchUIInit();
void touchInputInit();
void touchInputUpdate();
void resetUnlockPattern();

void displayInit();
void displaySplash(const char *text);
void displayUpdate();
void transitionTo(UIScreen next);

void drawScreensaver();
void drawUnlockScreen();
void drawMainMenu();
void drawManualFeedScreen();
void drawManualFeedDynamic(bool force);
void drawAutoModeScreen();

// Scale functions
void scaleInit();
void scaleTare();
float scaleRead();
void scaleCalibrate(float knownWeightG);

// Motor / Dispense functions
bool startDispense(float grams, const char *trigger, const String &commandId, const String &catId, const String &scheduleId);
void stopDispense();
void motorEmergencyStop();
void runDispensingCycle();
void checkScheduledFeeds();

// Time & Timezone
struct DeviceTime {
  bool valid;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t wday;  // 0 = Sunday, 6 = Saturday
  uint16_t yday; // 0 to 365
};
extern DeviceTime currentDeviceTime;

#define TIMEZONE_OFFSET_SEC 7200  // UTC+2 (e.g. Europe/Madrid summer time)
void updateTimeFromSupabase(const char* utcStr);
void updateLocalClock();

// Supabase REST calls
void sendHeartbeat();
void pollPendingCommands();
void pollDeviceConfig();
void publishRealtimeWeight(float weightG, float dispensedG, float targetG);
void logFeedEvent(const char *catId, float targetG, float actualG, const char *trigger, const char *status);
void updateCommandStatus(const String &cmdId, const char *status, float actualGrams);

#endif // CONFIG_H
