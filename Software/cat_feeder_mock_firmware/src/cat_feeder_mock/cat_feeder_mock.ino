/**
 * =============================================================
 *  Smart Cat Feeder — ESP32 WROOM-32 Mock Firmware
 *  Framework : Arduino (PlatformIO)
 *  Libraries : WiFi.h | PubSubClient | ArduinoJson v7
 * -------------------------------------------------------------
 *  Purpose:
 *    Simulates the cat feeder hardware (HX711, DHT22, DRV8825)
 *    over MQTT so the backend can be tested before physical
 *    assembly. No real sensors or motors are required.
 *
 *  MQTT Topics:
 *    PUBLISH  catfeeder/telemetry   — sensor data  (every 5 s)
 *    PUBLISH  catfeeder/alerts/jam  — motor jam     (every 60 s)
 *    SUBSCRIBE catfeeder/commands   — inbound cmds
 * =============================================================
 */
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
// ─────────────────────────────────────────────────────────────
//  ★  NETWORK & BROKER CONFIGURATION  — edit these values ★
// ─────────────────────────────────────────────────────────────
const char* SSID            = "iPhone de Ferran";       // 2.4 GHz network name
const char* PASSWORD        = "buti1234";   // Wi-Fi password
const char* MQTT_BROKER_IP  = "172.20.10.3";          // Your PC's local IP
const uint16_t MQTT_PORT    = 1883;
const char* MQTT_CLIENT_ID  = "esp32-cat-feeder-mock";
// ─────────────────────────────────────────────────────────────
// ── MQTT Topic Strings ────────────────────────────────────────
const char* TOPIC_TELEMETRY  = "catfeeder/telemetry";
const char* TOPIC_ALERT_JAM  = "catfeeder/alerts/jam";
const char* TOPIC_COMMANDS   = "catfeeder/commands";
// ── Publish Intervals ─────────────────────────────────────────
const unsigned long TELEMETRY_INTERVAL_MS = 5000;    //  5 seconds
const unsigned long ALERT_INTERVAL_MS     = 60000;   // 60 seconds
// ── Timing Trackers (millis-based, non-blocking) ──────────────
unsigned long lastTelemetryMs = 0;
unsigned long lastAlertMs     = 0;
// ── WiFi & MQTT Clients ───────────────────────────────────────
WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);
// =============================================================
//  FORWARD DECLARATIONS
// =============================================================
void setupWifi();
void reconnectMQTT();
void publishTelemetry();
void publishJamAlert();
void mqttCallback(char* topic, byte* payload, unsigned int length);
// =============================================================
//  Wi-Fi SETUP
//  Blocks until a connection is established. Prints progress
//  dots to Serial so you can track it in the monitor.
// =============================================================
void setupWifi() {
    Serial.println();
    Serial.print("[WiFi] Connecting to: ");
    Serial.println(SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("[WiFi] Connected! IP address: ");
    Serial.println(WiFi.localIP());
}
// =============================================================
//  MQTT RECONNECT
//  Called from loop() whenever the broker connection drops.
//  Retries every 5 s and subscribes to the commands topic on
//  a successful connection.
// =============================================================
void reconnectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("[MQTT] Connecting to broker at ");
        Serial.print(MQTT_BROKER_IP);
        Serial.print("...");
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.println(" connected.");
            // Subscribe to inbound command topic
            mqttClient.subscribe(TOPIC_COMMANDS);
            Serial.print("[MQTT] Subscribed to: ");
            Serial.println(TOPIC_COMMANDS);
        } else {
            Serial.print(" failed (rc=");
            Serial.print(mqttClient.state());
            Serial.println("). Retrying in 5 s...");
            delay(5000);
        }
    }
}
// =============================================================
//  MQTT CALLBACK — Inbound Command Handler
//  Triggered automatically by PubSubClient when a message
//  arrives on a subscribed topic.
//
//  Expected payload format:
//    { "action": "dispense", "amount_grams": 50 }
// =============================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.println();
    Serial.print("[CMD] Message received on topic: ");
    Serial.println(topic);
    // ── Copy raw bytes into a null-terminated string ──────────
    String rawPayload;
    rawPayload.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        rawPayload += (char)payload[i];
    }
    Serial.print("[CMD] Raw payload: ");
    Serial.println(rawPayload);
    // ── Deserialise JSON ──────────────────────────────────────
    JsonDocument doc;   // ArduinoJson v7: no size template arg needed
    DeserializationError err = deserializeJson(doc, rawPayload);
    if (err) {
        Serial.print("[CMD] JSON parse error: ");
        Serial.println(err.c_str());
        return;
    }
    // ── Extract and act on the 'action' field ─────────────────
    const char* action = doc["action"] | "unknown";
    Serial.print("[CMD] Action received: ");
    Serial.println(action);
    // Handle known actions
    if (strcmp(action, "dispense") == 0) {
        int amount = doc["amount_grams"] | 0;
        Serial.print("[CMD] → Simulating dispense of ");
        Serial.print(amount);
        Serial.println(" grams.");
    } else if (strcmp(action, "update_settings") == 0) {
        Serial.println("[CMD] → Simulating settings update.");
        // Additional fields can be read from doc here as needed
    } else if (strcmp(action, "tare_scale") == 0) {
        Serial.println("[CMD] → Simulating HX711 tare (zero weight).");
    } else {
        Serial.print("[CMD] → Unknown action '");
        Serial.print(action);
        Serial.println("' — ignoring.");
    }
    Serial.println();
}
// =============================================================
//  TELEMETRY PUBLISHER
//  Generates plausible fake sensor readings and publishes them
//  as a JSON object to catfeeder/telemetry.
//
//  Simulated Sensors:
//    • HX711 load cell  → weight        (100 – 500 g)
//    • DHT22            → temperature   (22 ± 2 °C)
//    • DHT22            → humidity      (45 ± 5 %)
//    • DRV8825 motor    → motor_status  ("idle")
//
//  JSON keys must match mqttService.js: { weight, temperature, humidity }
// =============================================================
void publishTelemetry() {
    // ── Generate fake readings ────────────────────────────────
    float weight      = (float)random(100, 501);                     // 100 – 500 g
    float temperature = 22.0f + ((float)random(-20, 21) / 10.0f);   // 22 ± 2 °C
    float humidity    = 45.0f + ((float)random(-50, 51) / 10.0f);   // 45 ± 5 %
    const char* motorStatus = "idle";
    // ── Serialise to JSON ─────────────────────────────────────
    // Keys must match exactly what mqttService.js destructures:
    //   const { weight, temperature, humidity } = data;
    JsonDocument doc;
    doc["weight"]       = weight;
    doc["temperature"]  = round(temperature * 10) / 10.0;  // 1 decimal place
    doc["humidity"]     = round(humidity * 10) / 10.0;
    doc["motor_status"] = motorStatus;
    char buffer[256];
    serializeJson(doc, buffer, sizeof(buffer));
    // ── Publish ───────────────────────────────────────────────
    bool ok = mqttClient.publish(TOPIC_TELEMETRY, buffer);
    Serial.print("[TELEMETRY] Published → ");
    Serial.print(buffer);
    Serial.println(ok ? " ✓" : " ✗ (publish failed)");
}
// =============================================================
//  JAM ALERT PUBLISHER
//  Simulates a motor jam event. In production this would be
//  triggered by a stall-detection algorithm on the DRV8825.
//
//  JSON key must match mqttService.js handleJamAlert(): { status }
// =============================================================
void publishJamAlert() {
    // mqttService.js reads: data.status
    const char* alertPayload = "{\"status\":\"jammed\"}";
    bool ok = mqttClient.publish(TOPIC_ALERT_JAM, alertPayload);
    Serial.print("[ALERT] Motor jam published → ");
    Serial.println(ok ? "✓" : "✗ (publish failed)");
}
// =============================================================
//  SETUP — runs once on boot
// =============================================================
void setup() {
    Serial.begin(115200);
    delay(500);  // Brief pause for the serial monitor to open
    Serial.println("==============================================");
    Serial.println("  Smart Cat Feeder — Mock Firmware Booting   ");
    Serial.println("==============================================");
    randomSeed(analogRead(0));  // Seed RNG from floating GPIO pin
    setupWifi();
    mqttClient.setServer(MQTT_BROKER_IP, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    // Increase buffer size to handle larger JSON payloads
    mqttClient.setBufferSize(512);
    Serial.println("[BOOT] Setup complete. Entering main loop.");
    Serial.println();
}
// =============================================================
//  LOOP — runs continuously
//  Uses millis()-based non-blocking timers; no delay() calls
//  that would block the MQTT client's internal keep-alive.
// =============================================================
void loop() {
    // ── Ensure Wi-Fi is still connected ──────────────────────
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Connection lost. Reconnecting...");
        setupWifi();
    }
    // ── Ensure MQTT broker is still connected ─────────────────
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    // ── Process incoming MQTT messages ────────────────────────
    mqttClient.loop();
    unsigned long now = millis();
    // ── Publish telemetry every 5 seconds ────────────────────
    if (now - lastTelemetryMs >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryMs = now;
        publishTelemetry();
    }
    // ── Publish jam alert every 60 seconds ───────────────────
    if (now - lastAlertMs >= ALERT_INTERVAL_MS) {
        lastAlertMs = now;
        publishJamAlert();
    }
}

