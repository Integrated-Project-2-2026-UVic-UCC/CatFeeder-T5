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

// PATCH /rest/v1/devices?device_id=eq.<id>  { last_seen, status, fw }
void sendHeartbeat() {
  StaticJsonDocument<192> doc;
  doc["last_seen"]       = "now()";
  doc["status"]          = cycle.active ? "dispensing" :
                           (currentState == STATE_ERROR ? "fault_motor" : "idle");
  doc["firmware_version"] = FW_VERSION;
  String body;
  serializeJson(doc, body);

  String ep = "/rest/v1/devices?device_id=eq." DEVICE_ID;
  int code  = httpPATCH(ep, body);
  if (code < 200 || code >= 300) {
    Serial.printf("[net] heartbeat HTTP %d\n", code);
  }
}

// POST /rest/v1/realtime_weight  { device_id, grams, at }
void pushRealtimeWeight(float grams) {
  StaticJsonDocument<160> doc;
  doc["device_id"] = DEVICE_ID;
  doc["grams"]     = grams;
  String body;
  serializeJson(doc, body);
  httpPOST("/rest/v1/realtime_weight", body);
}

// POST /rest/v1/feed_events  (SRS §5.2.3)
void logFeedEvent(const char* status, float dispensed) {
  StaticJsonDocument<384> doc;
  doc["device_id"]    = DEVICE_ID;
  if (cycle.catId.length())      doc["cat_id"]      = cycle.catId;
  if (cycle.scheduleId.length()) doc["schedule_id"] = cycle.scheduleId;
  doc["trigger_type"] = cycle.trigger;
  doc["target_grams"] = cycle.targetG;
  doc["actual_grams"] = dispensed;
  doc["status"]       = status;

  String body;
  serializeJson(doc, body);
  int code = httpPOST("/rest/v1/feed_events", body);
  if (code < 200 || code >= 300) {
    Serial.printf("[net] feed_event HTTP %d\n", code);
  }
}

// PATCH /rest/v1/commands?id=eq.<id>  { status }
void updateCommandStatus(const String& id, const char* status) {
  StaticJsonDocument<96> doc;
  doc["status"] = status;
  String body;
  serializeJson(doc, body);
  String ep = "/rest/v1/commands?id=eq." + id;
  httpPATCH(ep, body);
}

// --------------------------------------------------------------------------
// GET /rest/v1/commands?device_id=eq.<id>&status=eq.pending&select=id,cat_id,portion_grams
void pollPendingCommands() {
  if (cycle.active) return;    // ignore while dispensing

  String ep = "/rest/v1/commands?device_id=eq." DEVICE_ID
              "&status=eq.pending&select=id,cat_id,portion_grams&limit=1";
  String payload;
  int code = httpGET(ep, payload);
  if (code != 200 || payload.length() < 3) return;   // no rows

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err || !doc.is<JsonArray>() || doc.size() == 0) return;

  JsonObject cmd   = doc[0];
  String     id    = cmd["id"] | "";
  String     cat   = cmd["cat_id"] | "";
  float      grams = cmd["portion_grams"] | DEFAULT_PORTION_G;

  Serial.printf("[net] command %s, %.1f g\n", id.c_str(), grams);
  if (!startDispense(grams, "manual", id, cat, String(""))) {
    updateCommandStatus(id, "error");
  }
}

// --------------------------------------------------------------------------
// GET /rest/v1/device_config?device_id=eq.<id>&select=schedules,calibration
// The schedules field is a JSON array like:
//   [ { "id":"...", "cat_id":"...", "hour":8, "minute":30,
//       "days_of_week":[1,2,3,4,5], "portion_grams":25, "enabled":true }, ... ]
void pollDeviceConfig() {
  String ep = "/rest/v1/device_config?device_id=eq." DEVICE_ID
              "&select=schedules,calibration&limit=1";
  String payload;
  int code = httpGET(ep, payload);
  if (code != 200 || payload.length() < 3) return;

  StaticJsonDocument<4096> doc;
  if (deserializeJson(doc, payload)) return;
  if (!doc.is<JsonArray>() || doc.size() == 0) return;

  JsonArray sched = doc[0]["schedules"].as<JsonArray>();
  scheduleClear();
  for (JsonObject s : sched) {
    if (scheduleCount >= MAX_SCHEDULES) break;
    Schedule ns;
    ns.enabled  = s["enabled"]      | true;
    ns.hour     = s["hour"]         | 0;
    ns.minute   = s["minute"]       | 0;
    ns.portionG = s["portion_grams"]| DEFAULT_PORTION_G;
    ns.catId    = String((const char*)(s["cat_id"] | ""));
    ns.id       = String((const char*)(s["id"]     | ""));
    ns.lastFiredYday = 0;

    ns.daysMask = 0;
    JsonArray days = s["days_of_week"].as<JsonArray>();
    if (days.isNull() || days.size() == 0) {
      ns.daysMask = 0x7F;               // every day by default
    } else {
      for (int d : days) ns.daysMask |= (1 << (d & 0x07));
    }
    scheduleAdd(ns);
  }
  Serial.printf("[net] device_config: %u schedules\n", scheduleCount);
}
