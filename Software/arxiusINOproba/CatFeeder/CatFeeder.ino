// CatFeeder.ino — Main orchestrator
// Integrated Project II — GR15 [GEMEC-09UV]  Universitat de Vic — UCC
// ==========================================================================
// This sketch coordinates every physical subsystem of the automatic cat
// feeder: stepper dispenser, load cell (HX711), DHT22, DS3231 RTC, ILI9341
// touch display (touch-only UI) and the Supabase cloud link.
//
// The main loop is strictly non-blocking: all periodic tasks are driven by
// millis() timers and the stepper is stepped via AccelStepper::run().
// ==========================================================================

#include "config.h"

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>

#include <AccelStepper.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <HX711.h>
#include <RTClib.h>
#include <TFT_eSPI.h>

// Shared types referenced by auto-generated prototypes that the IDE hoists to
// the top of this (first) tab — must be visible here.
#include "schedules.h"

// ---------- Global peripheral objects (used from every *.ino tab) ----------
TFT_eSPI tft = TFT_eSPI();
HX711 scale;
DHT dht(DHT_PIN, DHT22);
RTC_DS3231 rtc;
AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STEP, STEPPER_DIR);

// ---------- System-wide state --------------------------------------------
enum DeviceState {
  STATE_BOOT,
  STATE_IDLE,
  STATE_DISPENSING,
  STATE_ERROR,
  STATE_OFFLINE_DEGRADED
};

DeviceState currentState = STATE_BOOT;
const char *lastErrorMessage = "";

// Live sensor snapshot (updated by updateSensors()).
struct Telemetry {
  float weightG; // current net weight on the tray
  float temperatureC;
  float humidity;
  bool wifiUp;
  bool rtcOk;
} telemetry = {0.0f, NAN, NAN, false, false};

// Active dispensing cycle descriptor.
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
} cycle = {false, 0, 0, 0, 0, 0, "manual", "", "", "", ""};

// Display screen currently visible (display phase will use this).
uint8_t activeScreen = 0; // 0 = dashboard, 1 = sensors, 2 = network

// ---- Cross-tab touch state (defined in TouchInput.ino) -------------------
extern UIMode   uiRequestedMode;
extern bool     uiManualFeedActive;
extern bool     touchDetected;
extern uint16_t touchX;
extern uint16_t touchY;

// ---- Cross-tab schedule state (defined in RTCManager.ino) ----------------
extern uint8_t  scheduleCount;

// Periodic-task timers.
static uint32_t tLastSensors = 0;
static uint32_t tLastDisplay = 0;
static uint32_t tLastHeartbeat = 0;
static uint32_t tLastCmdPoll = 0;
static uint32_t tLastCfgPoll = 0;
static uint32_t tLastWifiTry = 0;

// ==========================================================================
//                                   SETUP
// ==========================================================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(150);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("  CatFeeder — firmware " FW_VERSION));
  Serial.println(F("========================================"));

  // No physical buttons on this hardware — the UI is touch-only.

  // --- Stepper driver off until we really need it ---------------------------
  pinMode(STEPPER_EN, OUTPUT);
  digitalWrite(STEPPER_EN, HIGH); // disabled (active-low)

  // --- Bring up each subsystem ---------------------------------------------
  displayInit();
  displaySplash("Booting...");

  i2cInit();
  rtcInit();
  scaleInit();
  sensorsInit();
  motorInit();
  networkInit();
  touchInputInit();

  currentState = STATE_IDLE;
  displaySplash("Ready");
  delay(400);

  Serial.println(F("[boot] complete"));
}

// ==========================================================================
//                                   LOOP
// ==========================================================================
void loop() {
  const uint32_t now = millis();

  // 1) Always step the motor first — AccelStepper is cooperative.
  stepper.run();

  // 2) Touch input (read only; no rendering yet).
  touchInputUpdate();

  // 3) Manual-feed request from the touch panel.
  if (uiRequestedMode == UI_MODE_MANUAL) {
    if (uiManualFeedActive && currentState == STATE_IDLE) {
      startDispense(DEFAULT_PORTION_G, "manual", String(""), String(""), String(""));
    }
  }

  // 4) State machine.
  switch (currentState) {
  case STATE_IDLE:
    if (uiRequestedMode == UI_MODE_AUTO) {
      checkScheduledFeeds();
    }
    break;
  case STATE_DISPENSING:
    runDispensingCycle();
    if (now - cycle.lastPublishMs >= REALTIME_WEIGHT_INTERVAL_MS) {
      cycle.lastPublishMs = now;
      if (telemetry.wifiUp) {
        publishRealtimeWeight(telemetry.weightG, cycle.dispensedG, cycle.targetG);
      }
    }
    break;
  case STATE_ERROR:
    // Touch anywhere to acknowledge and clear the error.
    if (touchDetected) clearError();
    break;
  default:
    break;
  }

  // 5) Periodic sensor sampling.
  if (now - tLastSensors >= TELEMETRY_INTERVAL_MS) {
    tLastSensors = now;
    updateSensors();
  }

  // 6) Display refresh (no-op until the display phase).
  if (now - tLastDisplay >= DISPLAY_REFRESH_MS) {
    tLastDisplay = now;
    displayUpdate();
  }

  // 7) Network-bound tasks (skip cleanly when offline).
  if (WiFi.status() == WL_CONNECTED) {
    telemetry.wifiUp = true;
    if (now - tLastCmdPoll >= COMMAND_POLL_INTERVAL_MS) {
      tLastCmdPoll = now;
      pollPendingCommands();
    }
    if (now - tLastCfgPoll >= CONFIG_POLL_INTERVAL_MS) {
      tLastCfgPoll = now;
      pollDeviceConfig();
    }
    if (now - tLastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
      tLastHeartbeat = now;
      sendHeartbeat();
    }
  } else {
    telemetry.wifiUp = false;
    if (now - tLastWifiTry >= WIFI_RECONNECT_MS) {
      tLastWifiTry = now;
      networkReconnect();
    }
  }
}

// ==========================================================================
//                    Helpers exposed to every subsystem
// ==========================================================================
void i2cInit() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);
  Serial.println(F("[i2c] started"));
}

void goToError(const char *message) {
  lastErrorMessage = message;
  currentState = STATE_ERROR;
  motorEmergencyStop();
  Serial.print(F("[ERROR] "));
  Serial.println(message);
}

void clearError() {
  lastErrorMessage = "";
  currentState = STATE_IDLE;
  Serial.println(F("[ERROR] cleared"));
}

// Called by Motor.ino when a dispensing cycle is safely finished.
void cycleFinish(const char *status, float dispensed) {
  cycle.active = false;
  cycle.dispensedG = dispensed;
  motorEnable(false);

  // Log to Supabase only if we have connectivity.
  if (telemetry.wifiUp) {
    logFeedEvent(cycle.catId.c_str(), cycle.targetG, dispensed,
                 cycle.trigger, status);
    if (cycle.commandId.length() > 0) {
      updateCommandStatus(cycle.commandId, status, dispensed);
      cycle.commandId = "";
    }
  }

  currentState = STATE_IDLE;
  Serial.printf("[cycle] %s — %.1fg dispensed\n", status, dispensed);
}
