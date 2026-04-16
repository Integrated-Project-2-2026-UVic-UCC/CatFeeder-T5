#ifndef CONFIG_H
#define CONFIG_H

// ==========================================================================
// CatFeeder — Global Configuration
// Integrated Project II — GR15 [GEMEC-09UV]
// ==========================================================================
// Modify the credentials and pin map to match your hardware before flashing.
// ==========================================================================

// --------------------------- Firmware meta ---------------------------------
#define FW_VERSION              "0.1.0-base"
#define SERIAL_BAUD             115200

// --------------------------- WiFi ------------------------------------------
#define WIFI_SSID               "YOUR_WIFI_SSID"
#define WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"
#define WIFI_RECONNECT_MS       10000
#define WIFI_CONNECT_TIMEOUT_MS 15000

// --------------------------- Supabase (REST) -------------------------------
// See Software/SRS_CatFeeder_WebApp.docx.md §6 for the protocol.
#define SUPABASE_URL            "https://YOUR_PROJECT_REF.supabase.co"
#define SUPABASE_ANON_KEY       "eyJ...YOUR_SUPABASE_ANON_JWT..."
#define DEVICE_ID               "00000000-0000-0000-0000-000000000000"

// --------------------------- Display (ILI9488 SPI) -------------------------
// The concrete pins are defined in the TFT_eSPI User_Setup.h (see README).
// Suggested mapping (VSPI):
//   MOSI = 23  MISO = 19  SCLK = 18  CS = 5  DC = 2  RST = 4  BL = 3V3
// Display is used as OUTPUT ONLY — the touch panel is intentionally unused.
#define DISPLAY_ROTATION        1        // landscape 480x320

// --------------------------- I2C (RTC DS3231) ------------------------------
#define I2C_SDA                 21
#define I2C_SCL                 22
#define I2C_CLOCK_HZ            400000

// --------------------------- HX711 + Load Cell -----------------------------
#define HX711_DT                16
#define HX711_SCK               17
#define HX711_GAIN              128
#define HX711_SAMPLES           5
// Obtained via calibration routine (see Scale.ino -> scaleCalibrate()).
#define HX711_CALIBRATION_FACTOR  420.0f
#define SCALE_STABLE_THRESHOLD_G  0.3f

// --------------------------- DHT22 -----------------------------------------
#define DHT_PIN                 27
#define DHT_READ_INTERVAL_MS    2500     // DHT22 minimum sampling is ~2s

// --------------------------- Stepper NEMA 17 + DRV8825 / A4988 -------------
#define STEPPER_STEP            25
#define STEPPER_DIR             26
#define STEPPER_EN              33       // Active-LOW enable on most drivers
#define STEPPER_STEPS_PER_REV   200      // 1.8°/step motor
#define STEPPER_MICROSTEPS      16       // Wire MS1/MS2/MS3 accordingly
#define STEPPER_MAX_SPEED       2000.0f  // steps/s (after microstepping)
#define STEPPER_ACCEL           1500.0f  // steps/s^2
#define STEPPER_DISPENSE_SPEED  1200.0f  // nominal feed speed
#define STEPPER_MAX_RUN_MS      30000    // hard safety timeout per cycle

// Nominal grams per second at STEPPER_DISPENSE_SPEED.
// The control loop is gravimetric (it reads the scale), so this value is
// only used to predict time-to-target and detect blockages.
#define STEPPER_FEED_RATE_GPS   8.0f

// --------------------------- Buttons ---------------------------------------
// All buttons wired between GPIO and GND, using internal INPUT_PULLUP.
#define BTN_FEED                32       // Manual dispense
#define BTN_TARE                13       // Tare scale
#define BTN_MENU                14       // Cycle display screens
#define BTN_DEBOUNCE_MS         35

// --------------------------- Squishy Circuits switch -----------------------
// Soft conductive-dough contact. Used here as a "cat-present" confirmation
// input (cat touches the pad to request food).
#define SQUISHY_PIN             15
#define SQUISHY_DEBOUNCE_MS     120

// --------------------------- Feeder defaults -------------------------------
#define DEFAULT_PORTION_G       25.0f
#define MIN_PORTION_G           5.0f
#define MAX_PORTION_G           500.0f
#define FEED_TOLERANCE_G        1.5f

// --------------------------- Timings ---------------------------------------
#define TELEMETRY_INTERVAL_MS        2000
#define HEARTBEAT_INTERVAL_MS        30000
#define COMMAND_POLL_INTERVAL_MS     5000
#define CONFIG_POLL_INTERVAL_MS      60000
#define REALTIME_WEIGHT_INTERVAL_MS  500
#define DISPLAY_REFRESH_MS           250

#endif // CONFIG_H
