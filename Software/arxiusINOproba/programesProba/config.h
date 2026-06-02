#ifndef CONFIG_H
#define CONFIG_H

// ==========================================================================
// CatFeeder — Global Configuration
// Integrated Project II — GR15 [GEMEC-09UV]
// ==========================================================================
// Pin map matches the REAL soldered hardware (see project doc T5).
// ESP32 WROOM-32 + ILI9341 2.8" 320x240 (XPT2046 touch, shared SPI bus)
// HX711 load cell, DHT22, DS3231 RTC, NEMA17 + DRV8825.
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

// --------------------------- I2C (RTC DS3231) ------------------------------
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

// --------------------------- Touch UI mode ---------------------------------
// Defined here (dependency-free) so every .ino tab sees it regardless of the
// order in which the Arduino IDE concatenates the tabs.
enum UIMode {
  UI_MODE_MANUAL,
  UI_MODE_AUTO
};

#endif // CONFIG_H
