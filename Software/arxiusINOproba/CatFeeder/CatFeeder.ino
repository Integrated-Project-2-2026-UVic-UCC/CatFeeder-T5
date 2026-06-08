// CatFeeder.ino — Main orchestrator
// Integrated Project II — GR15 [GEMEC-09UV]  Universitat de Vic — UCC
// ==========================================================================
// This sketch coordinates every physical subsystem of the automatic cat
// feeder: stepper dispenser, load cell (HX711), DHT22, ILI9341
// touch display (touch-only UI) and the Supabase cloud link.
// NOTE: DS3231 RTC removed to avoid I2C conflicts; time is managed via
// millis() and NTP-sourced timestamps from Supabase are used instead.
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
#include <TFT_eSPI.h>

// Shared types referenced by auto-generated prototypes that the IDE hoists to
// the top of this (first) tab — must be visible here.
#include "schedules.h"

// ---------- Global peripheral objects (used from every *.ino tab) ----------
TFT_eSPI tft = TFT_eSPI();
HX711 scale;
DHT dht(DHT_PIN, DHT22);
AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STEP, STEPPER_DIR);

// ---------- System-wide state --------------------------------------------
DeviceState currentState = STATE_BOOT;
const char *lastErrorMessage = "";

// Live sensor snapshot (updated by updateSensors()).
Telemetry telemetry = {0.0f, NAN, NAN, false};

// Active dispensing cycle descriptor.
FeedingCycle cycle = {false, 0.0f, 0.0f, 0.0f, 0, 0, "manual", "", "", "", ""};

// ---- Cross-tab schedule state (defined in ScheduleManager.ino) -----------
extern uint8_t  scheduleCount;

// Periodic-task timers.
static uint32_t tLastSensors      = 0;
static uint32_t tLastDisplay      = 0;
static uint32_t tLastHeartbeat    = 0;
static uint32_t tLastCmdPoll      = 0;
static uint32_t tLastCfgPoll      = 0;
static uint32_t tLastWifiTry      = 0;
static uint32_t tLastWeightPublish = 0;   // publish weight even when idle
#define IDLE_WEIGHT_PUBLISH_MS 10000      // every 10 s in idle state

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

  touchInputInit();

  displayInit();
  displaySplash("Booting...");

  i2cInit();
  scaleInit();
  sensorsInit();
  motorInit();
  networkInit();

  touchUIInit();

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

  // Motor stepping is now handled inside runDispensingCycle() via direct GPIO.
  // (AccelStepper kept in headers for motorInit compatibility but not used
  //  for dispensing — direct micros()-based pulses match Test_Motor.ino.)

  // 2) Touch input (reads touchscreen and drives UI screen state).
  static uint32_t tLastTouchUpdate = 0;
  if (now - tLastTouchUpdate >= 60) {
    tLastTouchUpdate = now;
    touchInputUpdate();
  }

  // 3) State machine.
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
    // Handled in touchInputUpdate(): clicking anywhere clears the error state.
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
    if (!telemetry.wifiUp) {
      telemetry.wifiUp = true;
      // WiFi just connected: sync heartbeat (gets time) and configs immediately
      tLastHeartbeat = now;
      sendHeartbeat();
      tLastCfgPoll = now;
      pollDeviceConfig();
      tLastCmdPoll = now;
      pollPendingCommands();
    }
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
    // Publish current weight every 10 s even when idle so the webapp
    // "Live Sensor Readings" always shows an up-to-date value.
    if (!cycle.active && now - tLastWeightPublish >= IDLE_WEIGHT_PUBLISH_MS) {
      tLastWeightPublish = now;
      publishRealtimeWeight(telemetry.weightG, 0, 0);
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
