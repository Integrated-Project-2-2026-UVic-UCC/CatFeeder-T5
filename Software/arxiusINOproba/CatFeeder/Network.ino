// ==========================================================================
// Network.ino — WiFi + Supabase REST client
// ==========================================================================
// Implements the minimum of the protocol described in SRS §6:
//   - Poll `commands`       for pending manual dispenses
//   - Poll `device_config`  for schedules / motor limits / calibration
//   - PATCH `devices` heartbeat
//   - POST  `realtime_weight` during a cycle
//   - POST  `feed_events`   when a cycle ends
//   - PATCH `commands`      to close them
// TLS is used but certificate validation is skipped for the base code —
// production builds should pin the Supabase CA bundle.
// ==========================================================================

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

static WiFiClientSecure netClient;

// Defined in RTCManager.ino; declared here so the JsonArray& signature is
// visible at the call site (Arduino auto-prototyping skips library types).
void loadSchedules(JsonArray &arr);
extern uint8_t scheduleCount;

static const char* HDR_APIKEY = "apikey";
static const char* HDR_AUTH   = "Authorization";

// --------------------------------------------------------------------------
void networkInit() {
  netClient.setInsecure();                  // TODO: pin Supabase CA for prod

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[wifi] connecting to %s ", WIFI_SSID);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    Serial.print('.');
    delay(250);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] ok, IP=%s\n", WiFi.localIP().toString().c_str());
    telemetry.wifiUp = true;
  } else {
    Serial.println(F("[wifi] failed — continuing offline"));
    telemetry.wifiUp = false;
  }
}

// --------------------------------------------------------------------------
void networkReconnect() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.println(F("[wifi] reconnecting..."));
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// ---------------- low-level helpers ---------------------------------------
static void applySupabaseHeaders(HTTPClient& http, bool patch = false) {
  http.addHeader(HDR_APIKEY, SUPABASE_ANON_KEY);
  http.addHeader(HDR_AUTH,   String("Bearer ") + SUPABASE_ANON_KEY);
  http.addHeader("Content-Type", "application/json");
  if (patch) http.addHeader("Prefer", "return=minimal");
}

static int httpGET(const String& endpoint, String& out) {
  if (WiFi.status() != WL_CONNECTED) return -1;
  HTTPClient http;
  http.begin(netClient, String(SUPABASE_URL) + endpoint);
  applySupabaseHeaders(http);
  int code = http.GET();
  if (code > 0) out = http.getString();
  http.end();
  return code;
}

static int httpPOST(const String& endpoint, const String& body) {
  if (WiFi.status() != WL_CONNECTED) return -1;
  HTTPClient http;
  http.begin(netClient, String(SUPABASE_URL) + endpoint);
  applySupabaseHeaders(http);
  int code = http.POST(body);
  http.end();
  return code;
}

static int httpPATCH(const String& endpoint, const String& body) {
  if (WiFi.status() != WL_CONNECTED) return -1;
  HTTPClient http;
  http.begin(netClient, String(SUPABASE_URL) + endpoint);
  applySupabaseHeaders(http, true);
  int code = http.PATCH(body);
  http.end();
  return code;
}

// ==========================================================================
//                             Protocol calls
// ==========================================================================

// Builds an ISO-8601 UTC timestamp from the DS3231. Falls back to "now"
// (PostgREST accepts it for now()) when the RTC is unavailable.
static String isoTimestamp() {
  if (!telemetry.rtcOk) return String("now");
  DateTime t = rtc.now();
  char buf[32];
  snprintf(buf, sizeof(buf), "%04u-%02u-%02uT%02u:%02u:%02uZ",
           t.year(), t.month(), t.day(), t.hour(), t.minute(), t.second());
  return String(buf);
}

// PATCH /rest/v1/devices?id=eq.<id>  { last_seen, status, fw }
void sendHeartbeat() {
  JsonDocument doc;
  doc["last_seen"]       = "now";
  doc["status"]          = cycle.active ? "dispensing" :
                           (currentState == STATE_ERROR ? "fault_motor" : "idle");
  doc["firmware_version"] = FW_VERSION;
  String body;
  serializeJson(doc, body);

  String ep = "/rest/v1/devices?id=eq." DEVICE_ID;
  int code  = httpPATCH(ep, body);
  if (code < 200 || code >= 300) {
    Serial.printf("[net] heartbeat HTTP %d\n", code);
  }
}

// POST /rest/v1/realtime_weight  (upsert by device_id)
//   { device_id, weight_g, dispensed_g, target_g, updated_at }
void publishRealtimeWeight(float weightG, float dispensedG, float targetG) {
  if (WiFi.status() != WL_CONNECTED) return;
  JsonDocument doc;
  doc["device_id"]   = DEVICE_ID;
  doc["weight_g"]    = weightG;
  doc["dispensed_g"] = dispensedG;
  doc["target_g"]    = targetG;
  doc["updated_at"]  = isoTimestamp();
  String body;
  serializeJson(doc, body);

  HTTPClient http;
  http.begin(netClient, String(SUPABASE_URL) + "/rest/v1/realtime_weight");
  applySupabaseHeaders(http);
  http.addHeader("Prefer", "resolution=merge-duplicates"); // upsert
  int code = http.POST(body);
  http.end();
  if (code < 200 || code >= 300) {
    Serial.printf("[net] realtime_weight HTTP %d\n", code);
  }
}

// POST /rest/v1/feed_events
//   { device_id, cat_id, target_grams, actual_grams, trigger_type,
//     status, started_at }
void logFeedEvent(const char *catId, float targetG, float actualG,
                  const char *trigger, const char *status) {
  JsonDocument doc;
  doc["device_id"]    = DEVICE_ID;
  if (catId && catId[0])              doc["cat_id"] = catId;       // null if absent
  if (cycle.scheduleId.length())      doc["schedule_id"] = cycle.scheduleId;
  doc["target_grams"] = targetG;
  doc["actual_grams"] = actualG;
  doc["trigger_type"] = trigger;     // "manual" or "scheduled"
  doc["status"]       = status;      // "completed" or "error"
  doc["started_at"]   = cycle.startedAtIso;

  String body;
  serializeJson(doc, body);
  int code = httpPOST("/rest/v1/feed_events", body);
  if (code < 200 || code >= 300) {
    Serial.printf("[net] feed_event HTTP %d\n", code);
  }
}

// PATCH /rest/v1/commands?id=eq.<id>  { status, actual_grams }
void updateCommandStatus(const String &cmdId, const char *status,
                         float actualGrams) {
  JsonDocument doc;
  doc["status"]       = status;
  doc["actual_grams"] = actualGrams;
  String body;
  serializeJson(doc, body);
  String ep = "/rest/v1/commands?id=eq." + cmdId;
  httpPATCH(ep, body);
}

// --------------------------------------------------------------------------
// GET /rest/v1/commands?device_id=eq.<id>&status=eq.pending
//      &select=id,cat_id,portion_grams,trigger_type&limit=1
void pollPendingCommands() {
  String ep = "/rest/v1/commands?device_id=eq." DEVICE_ID
              "&status=eq.pending&select=id,cat_id,portion_grams,trigger_type"
              "&limit=1";
  String payload;
  int code = httpGET(ep, payload);
  if (code != 200 || payload.length() < 3) return;   // no rows

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err || !doc.is<JsonArray>() || doc.size() == 0) return;

  // Only accept a command when the device is idle.
  if (currentState != STATE_IDLE) return;

  JsonObject cmd   = doc[0];
  String     id    = cmd["id"]            | "";
  String     cat   = cmd["cat_id"]        | "";
  float      grams = cmd["portion_grams"] | DEFAULT_PORTION_G;

  if (id.length() == 0) return;

  Serial.printf("[net] command %s, %.1f g\n", id.c_str(), grams);

  // Claim the command before acting on it.
  {
    JsonDocument up;
    up["status"] = "processing";
    String body;
    serializeJson(up, body);
    httpPATCH("/rest/v1/commands?id=eq." + id, body);
  }

  if (!startDispense(grams, "manual", id, cat, String(""))) {
    updateCommandStatus(id, "error", 0.0f);
  }
}

// GET /rest/v1/schedules?device_id=eq.<id>&enabled=eq.true
//      &select=id,cat_id,time_of_day,days_of_week,portion_grams
void pollDeviceConfig() {
  String ep = "/rest/v1/schedules?device_id=eq." DEVICE_ID
              "&enabled=eq.true"
              "&select=id,cat_id,time_of_day,days_of_week,portion_grams";
  String payload;
  int code = httpGET(ep, payload);
  if (code != 200 || payload.length() < 2) return;

  JsonDocument doc;
  if (deserializeJson(doc, payload)) return;
  if (!doc.is<JsonArray>()) return;

  JsonArray arr = doc.as<JsonArray>();
  loadSchedules(arr);   // implemented in RTCManager.ino
  Serial.printf("[net] schedules loaded: %u\n", scheduleCount);
}
