/*
 ============================================================
  CatFeeder Mock Firmware — ESP32 WROOM-32
  Integrated Project II · GR15 [GEMEC-09UV] · UVic-UCC
  Academic Year 2025-2026
 ============================================================
  PURPOSE:
    Simulates all hardware sensors and actuators of the smart
    cat feeder to allow full web-app testing WITHOUT physical
    hardware. Replace the simulation blocks with real sensor
    calls once the electronics are assembled.

  HARDWARE TO INTEGRATE LATER:
    - HX711 load cell ADC  (food weight)
    - DHT22 temperature & humidity sensor
    - DRV8825 stepper motor driver (auger motor)
    - Status LED
    - Optional: RFID RC522 for cat identification

  COMMUNICATION:
    - WiFi (STA mode) → internet or LAN
    - Supabase REST API over HTTPS (HTTPClient)
    - Polls device_config every 60 s
    - Posts heartbeat (devices table) every 30 s
    - Posts realtime_weight every 500 ms during dispensing
    - Polls commands table for pending commands

  DEPENDENCIES (install via Arduino Library Manager):
    - ArduinoJson  >= 7.x  (https://arduinojson.org)
    - WiFiClientSecure (built-in ESP32 Arduino core)
    - HTTPClient       (built-in ESP32 Arduino core)

  BOARD SETUP:
    - Board: "ESP32 Dev Module" (or WROOM-32)
    - Flash: 4 MB, Partition: Default
    - Upload speed: 115200

  USAGE:
    1. Fill in WIFI_SSID, WIFI_PASS, DEVICE_ID below.
    2. The SUPABASE_URL and SUPABASE_ANON_KEY match the webapp.
    3. Upload to ESP32, open Serial Monitor at 115200 baud.
 ============================================================
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ──────────────────────────────────────────────
//  USER CONFIGURATION — edit before uploading
// ──────────────────────────────────────────────
const char *WIFI_SSID = "Lab-Modul";
const char *WIFI_PASS = "GVe836Nf";

// Supabase project (matches webapp .env)
const char *SUPABASE_URL = "https://jawqxuzlvvzsrobftupx.supabase.co";
// ⚠️  IMPORTANT: Use the anon JWT key (NOT the publishable key
// sb_publishable_...).
//     The REST API requires a JWT in the Authorization header.
//     Get this from: Supabase Dashboard → Project → Settings → API →
//     anon/public
const char *SUPABASE_KEY =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imphd3F4dXpsdnZ6c3JvYmZ0dXB4Iiwicm9sZSI6Im"
    "Fub24iLCJpYXQiOjE3NzM4NjY2MjcsImV4cCI6MjA4OTQ0MjYyN30."
    "FnvhOpqhZh9Z3j2XkIPla1wUbx3wAsaP4anr44Utrzs";

// Register a device in the webapp first, then paste the UUID here
const char *DEVICE_ID = "8a96c22a-1b29-47a8-8ca1-0f47e5aa2edd";

// ──────────────────────────────────────────────
//  SIMULATION PARAMETERS
// ──────────────────────────────────────────────
// Set to true to enable verbose serial logging
#define DEBUG_LOG true

// Simulated sensor noise bands
const float WEIGHT_NOISE_G = 1.5f; // ±g random variation per reading
const float TEMP_BASE_C = 23.0f;   // base ambient temperature
const float TEMP_NOISE_C = 0.8f;
const float HUM_BASE_PCT = 48.0f;
const float HUM_NOISE_PCT = 2.0f;

// Motor simulation
const float DISPENSE_RATE_G_S = 8.0f; // simulated grams per second dispensed

// ──────────────────────────────────────────────
//  GLOBAL STATE
// ──────────────────────────────────────────────
enum DeviceState {
  STATE_IDLE,
  STATE_DISPENSING,
  STATE_FAULT_MOTOR,
  STATE_FAULT_SENSOR
};

DeviceState g_state = STATE_IDLE;
float g_currentWeight = 0.0f;
float g_targetWeight = 0.0f;
float g_dispensedWeight = 0.0f;
String g_activeCmdId = "";
String g_activeCatId = "";
bool g_tarePending = false;
bool g_restartPending = false;

// Config from Supabase
int g_configVersion = -1;
float g_calibrationFactor = 1.0f;
int g_motorTimeout = 30;
float g_hopperCapacity = 2000.0f;

// Timers
unsigned long g_lastHeartbeat = 0;
unsigned long g_lastConfigPoll = 0;
unsigned long g_lastCmdPoll = 0;
unsigned long g_lastWeightPost = 0;
unsigned long g_dispenseStart = 0;

// Intervals (ms)
const unsigned long HEARTBEAT_INTERVAL = 30000;
const unsigned long CONFIG_INTERVAL = 60000;
const unsigned long CMD_POLL_INTERVAL = 5000;
const unsigned long WEIGHT_POST_INTERVAL = 500;

// ──────────────────────────────────────────────
//  UTILITY: random float in range
// ──────────────────────────────────────────────
float randRange(float lo, float hi) {
  return lo + (float)random(0, 10000) / 10000.0f * (hi - lo);
}

// ──────────────────────────────────────────────
//  SENSOR SIMULATION FUNCTIONS
//  → Replace these with real sensor reads later
// ──────────────────────────────────────────────

/**
 * readWeight() — simulates HX711 load cell reading
 *
 * REAL IMPLEMENTATION (replace this block):
 *   #include "HX711.h"
 *   HX711 scale;
 *   scale.begin(DOUT_PIN, CLK_PIN);
 *   scale.set_scale(g_calibrationFactor);
 *   return scale.get_units(5);
 */
float readWeight() {
  float noise = randRange(-WEIGHT_NOISE_G, WEIGHT_NOISE_G);
  return max(0.0f, g_currentWeight + noise);
}

/**
 * readTemperature() — simulates DHT22 temperature sensor
 *
 * REAL IMPLEMENTATION:
 *   #include "DHT.h"
 *   DHT dht(DHT_PIN, DHT22);
 *   return dht.readTemperature();
 */
float readTemperature() {
  return TEMP_BASE_C + randRange(-TEMP_NOISE_C, TEMP_NOISE_C);
}

/**
 * readHumidity() — simulates DHT22 humidity sensor
 *
 * REAL IMPLEMENTATION:
 *   return dht.readHumidity();
 */
float readHumidity() {
  return HUM_BASE_PCT + randRange(-HUM_NOISE_PCT, HUM_NOISE_PCT);
}

/**
 * setMotorRunning() — simulates DRV8825 stepper motor control
 *
 * REAL IMPLEMENTATION:
 *   digitalWrite(MOTOR_ENABLE_PIN, running ? LOW : HIGH);
 *   if (running) { step motor via STEP/DIR pins }
 */
void setMotorRunning(bool running) {
  if (DEBUG_LOG)
    Serial.printf("[MOTOR] %s\n", running ? "START" : "STOP");
  // TODO: digitalWrite(MOTOR_ENABLE_PIN, running ? LOW : HIGH);
}

/**
 * performTare() — simulates zeroing the scale
 *
 * REAL IMPLEMENTATION:
 *   scale.tare();
 */
void performTare() {
  g_currentWeight = 0.0f;
  g_dispensedWeight = 0.0f;
  if (DEBUG_LOG)
    Serial.println("[TARE] Scale zeroed");
  // TODO: scale.tare();
}

// ──────────────────────────────────────────────
//  HTTP HELPERS
// ──────────────────────────────────────────────
String buildUrl(const char *path) {
  return String(SUPABASE_URL) + "/rest/v1/" + path;
}

bool supabaseGet(const String &path, JsonDocument &out) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, buildUrl(path.c_str()));
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  int code = http.GET();
  String payload = http.getString();
  http.end();
  if (code == 200) {
    deserializeJson(out, payload);
    return true;
  }
  if (DEBUG_LOG)
    Serial.printf("[HTTP GET ERROR] %s → %d | %s\n", path.c_str(), code,
                  payload.c_str());
  return false;
}

bool supabasePost(const String &path, const String &body) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, buildUrl(path.c_str()));
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");

  int code = http.POST(body);
  bool ok = (code >= 200 && code < 300);
  if (!ok && DEBUG_LOG) {
    Serial.printf("[HTTP POST ERROR] %s → %d\n  Body: %s\n  Resp: %s\n",
                  path.c_str(), code, body.c_str(), http.getString().c_str());
  } else if (DEBUG_LOG) {
    Serial.printf("[HTTP POST OK] %s → %d\n", path.c_str(), code);
  }
  http.end();
  return ok;
}

bool supabasePatch(const String &path, const String &body) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, buildUrl(path.c_str()));
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");

  int code = http.PATCH(body);
  bool ok = (code >= 200 && code < 300);
  if (!ok && DEBUG_LOG) {
    Serial.printf("[HTTP PATCH ERROR] %s → %d\n  Body: %s\n  Resp: %s\n",
                  path.c_str(), code, body.c_str(), http.getString().c_str());
  } else if (DEBUG_LOG) {
    Serial.printf("[HTTP PATCH OK] %s → %d\n", path.c_str(), code);
  }
  http.end();
  return ok;
}

// ──────────────────────────────────────────────
//  SUPABASE OPERATIONS
// ──────────────────────────────────────────────

void postHeartbeat() {
  if (strlen(DEVICE_ID) < 10)
    return; // guard: no device ID
  String status;
  switch (g_state) {
  case STATE_IDLE:
    status = "idle";
    break;
  case STATE_DISPENSING:
    status = "dispensing";
    break;
  case STATE_FAULT_MOTOR:
    status = "fault_motor";
    break;
  case STATE_FAULT_SENSOR:
    status = "fault_sensor";
    break;
  }
  // Use a fixed reference epoch + millis() offset to approximate ISO time
  // For real hardware, use NTP: configTime(0,0,"pool.ntp.org") + getLocalTime()
  String body = "{\"status\":\"" + status +
                "\","
                "\"firmware_version\":\"mock-1.0.0\"}";
  supabasePatch(String("devices?id=eq.") + DEVICE_ID, body);
}

void postRealtimeWeight(float weight) {
  if (strlen(DEVICE_ID) < 10)
    return;
  String body = "{\"device_id\":\"" + String(DEVICE_ID) +
                "\","
                "\"weight_grams\":" +
                String(weight, 2) + "}";
  supabasePost("realtime_weight", body);
}

void postFeedEvent(float actual, const String &status,
                   const String &errCode = "") {
  if (strlen(DEVICE_ID) < 10)
    return;
  String body =
      "{\"device_id\":\"" + String(DEVICE_ID) +
      "\","
      "\"cat_id\":" +
      (g_activeCatId.length() ? "\"" + g_activeCatId + "\"" : "null") +
      ","
      "\"trigger_type\":\"manual\","
      "\"target_grams\":" +
      String(g_targetWeight, 1) +
      ","
      "\"actual_grams\":" +
      String(actual, 1) +
      ","
      "\"status\":\"" +
      status + "\"" +
      (errCode.length() ? ",\"error_code\":\"" + errCode + "\"" : "") + "}";
  supabasePost("feed_events", body);
}

void markCommandStatus(const String &cmdId, const String &status) {
  if (cmdId.length() < 10)
    return;
  String body = "{\"status\":\"" + status + "\"}";
  supabasePatch(String("commands?id=eq.") + cmdId, body);
}

void pollConfig() {
  if (strlen(DEVICE_ID) < 10)
    return;
  JsonDocument doc;
  if (!supabaseGet(
          String("device_config?device_id=eq.") + DEVICE_ID + "&limit=1", doc))
    return;
  if (doc.as<JsonArray>().size() == 0)
    return;

  JsonObject cfg = doc[0];
  int ver = cfg["config_version"] | 1;
  if (ver == g_configVersion)
    return; // no change

  g_configVersion = ver;
  g_calibrationFactor = cfg["calibration_factor"] | 1.0f;
  g_motorTimeout = cfg["motor_timeout_seconds"] | 30;
  g_hopperCapacity = cfg["hopper_capacity_grams"] | 2000.0f;
  bool tareTrigger = cfg["tare_trigger"] | false;

  if (tareTrigger)
    g_tarePending = true;

  if (DEBUG_LOG)
    Serial.printf("[CONFIG] v%d loaded · cal=%.4f · timeout=%ds\n",
                  g_configVersion, g_calibrationFactor, g_motorTimeout);
}

void pollCommands() {
  if (strlen(DEVICE_ID) < 10)
    return;
  if (g_state == STATE_DISPENSING)
    return; // busy — skip

  JsonDocument doc;
  String path = String("commands?device_id=eq.") + DEVICE_ID +
                "&status=eq.pending&order=created_at.asc&limit=1";
  if (!supabaseGet(path, doc))
    return;
  if (doc.as<JsonArray>().size() == 0)
    return;

  JsonObject cmd = doc[0];
  String cmdId = cmd["id"].as<String>();
  String cmdType = cmd["command_type"].as<String>();

  if (DEBUG_LOG)
    Serial.printf("[CMD] Received: %s (id=%s)\n", cmdType.c_str(),
                  cmdId.c_str());

  if (cmdType == "feed") {
    float portion = cmd["portion_grams"] | 50.0f;
    g_activeCatId = cmd["cat_id"].as<String>();
    g_activeCmdId = cmdId;
    g_targetWeight = portion;
    g_dispensedWeight = 0.0f;
    g_dispenseStart = millis();
    g_state = STATE_DISPENSING;
    setMotorRunning(true);
    Serial.printf("[DISPENSE] Starting: %.1fg for cat %s\n", portion,
                  g_activeCatId.c_str());

  } else if (cmdType == "tare") {
    performTare();
    markCommandStatus(cmdId, "completed");

  } else if (cmdType == "restart") {
    markCommandStatus(cmdId, "completed");
    Serial.println("[RESTART] Restarting in 2s…");
    delay(2000);
    ESP.restart();
  }
}

// ──────────────────────────────────────────────
//  DISPENSING SIMULATION LOOP
// ──────────────────────────────────────────────
void updateDispensing() {
  float elapsed = (millis() - g_dispenseStart) / 1000.0f;
  g_dispensedWeight = min(g_targetWeight, elapsed * DISPENSE_RATE_G_S);
  g_currentWeight = g_dispensedWeight;

  // Check motor timeout (safety limit)
  if (elapsed > g_motorTimeout) {
    Serial.println("[FAULT] Motor timeout exceeded!");
    setMotorRunning(false);
    postFeedEvent(g_dispensedWeight, "partial", "motor_timeout");
    markCommandStatus(g_activeCmdId, "error");
    g_state = STATE_FAULT_MOTOR;
    return;
  }

  // Check if target reached (within 1g tolerance)
  if (g_dispensedWeight >= g_targetWeight - 0.5f) {
    setMotorRunning(false);
    float actual = g_dispensedWeight + randRange(-0.5f, 0.5f);
    Serial.printf("[DISPENSE] Complete! Dispensed: %.1fg (target %.1fg)\n",
                  actual, g_targetWeight);
    postFeedEvent(actual, "completed");
    markCommandStatus(g_activeCmdId, "completed");
    g_state = STATE_IDLE;
    g_activeCmdId = "";
  }
}

// ──────────────────────────────────────────────
//  SETUP
// ──────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n╔══════════════════════════════════╗");
  Serial.println("║  CatFeeder Mock Firmware v1.0.0  ║");
  Serial.println("║  ESP32 WROOM-32  ·  GR15 UVic    ║");
  Serial.println("╚══════════════════════════════════╝\n");

  // WiFi connection
  Serial.printf("[WiFi] Connecting to %s…\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected! IP: %s\n",
                  WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] FAILED — running in offline simulation mode");
  }

  randomSeed(analogRead(36)); // use floating pin for entropy

  // Initial data load
  if (WiFi.status() == WL_CONNECTED) {
    pollConfig();
    postHeartbeat();
  }

  Serial.println("[SETUP] Ready. Polling Supabase every 5s for commands.");
}

// ──────────────────────────────────────────────
//  MAIN LOOP
// ──────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── WiFi reconnect guard ──
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Reconnecting…");
    WiFi.reconnect();
    delay(5000);
    return;
  }

  // ── Heartbeat ──
  if (now - g_lastHeartbeat >= HEARTBEAT_INTERVAL) {
    g_lastHeartbeat = now;
    postHeartbeat();
  }

  // ── Config poll ──
  if (now - g_lastConfigPoll >= CONFIG_INTERVAL) {
    g_lastConfigPoll = now;
    pollConfig();
  }

  // ── Command poll ──
  if (now - g_lastCmdPoll >= CMD_POLL_INTERVAL) {
    g_lastCmdPoll = now;
    pollCommands();
  }

  // ── Dispensing update ──
  if (g_state == STATE_DISPENSING) {
    updateDispensing();

    // Post live weight every 500 ms
    if (now - g_lastWeightPost >= WEIGHT_POST_INTERVAL) {
      g_lastWeightPost = now;
      float w = readWeight();
      postRealtimeWeight(w);
      if (DEBUG_LOG)
        Serial.printf("[WEIGHT] %.2fg\n", w);
    }
  }

  // ── Tare pending ──
  if (g_tarePending) {
    performTare();
    g_tarePending = false;
    // Update config to clear tare_trigger flag
    if (strlen(DEVICE_ID) >= 10) {
      supabasePatch(String("device_config?device_id=eq.") + DEVICE_ID,
                    "{\"tare_trigger\":false}");
    }
  }

  // ── Fault recovery: auto-clear motor fault after 60s ──
  if (g_state == STATE_FAULT_MOTOR && now - g_dispenseStart > 60000) {
    Serial.println("[FAULT] Auto-recovering from motor fault");
    g_state = STATE_IDLE;
  }

  // ── Serial debug info (and idle weight post) every 10s ──
  static unsigned long lastPrint = 0;
  if (now - lastPrint >= 10000) {
    lastPrint = now;
    float temp = readTemperature();
    float hum = readHumidity();
    float w = readWeight();
    Serial.printf("[SYS] State=%d | Weight=%.1fg | Temp=%.1f°C | Hum=%.1f%%\n",
                  (int)g_state, w, temp, hum);

    // Post weight so the web app shows it even when idle
    if (g_state != STATE_DISPENSING) {
      postRealtimeWeight(w);
    }
  }

  delay(50); // small yield to prevent WDT issues
}
