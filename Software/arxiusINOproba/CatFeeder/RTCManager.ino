// ==========================================================================
// RTCManager.ino — DS3231 real-time clock + schedule evaluator
// ==========================================================================
// Schedules live in memory here; they are refreshed from device_config in
// Network.ino (pollDeviceConfig). Everything is checked locally so feeding
// keeps working when the network is down (SRS §REQ-NF-10).
// ==========================================================================

#include <ArduinoJson.h>
#include "schedules.h"   // Schedule struct + MAX_SCHEDULES

Schedule schedules[MAX_SCHEDULES];
uint8_t  scheduleCount = 0;

// --------------------------------------------------------------------------
void rtcInit() {
  if (!rtc.begin()) {
    Serial.println(F("[rtc] DS3231 not found"));
    telemetry.rtcOk = false;
    return;
  }
  if (rtc.lostPower()) {
    Serial.println(F("[rtc] lost power, defaulting to compile time"));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  telemetry.rtcOk = true;
  Serial.println(F("[rtc] DS3231 ready"));
}

// --------------------------------------------------------------------------
// Clears in-memory schedules. Called before re-hydrating from device_config.
void scheduleClear() {
  scheduleCount = 0;
}

// Append one schedule (called from Network.ino while parsing JSON).
void scheduleAdd(const Schedule& s) {
  if (scheduleCount >= MAX_SCHEDULES) return;
  schedules[scheduleCount++] = s;
}

// --------------------------------------------------------------------------
// Rehydrate the in-memory schedule list from a Supabase `schedules` array:
//   [ { "id":..,"cat_id":..,"time_of_day":"HH:MM:SS",
//       "days_of_week":[0..6],"portion_grams":25 }, ... ]
// days_of_week ints follow DS3231 dayOfTheWeek(): 0=Sun .. 6=Sat.
void loadSchedules(JsonArray& arr) {
  scheduleClear();
  for (JsonObject s : arr) {
    if (scheduleCount >= MAX_SCHEDULES) break;

    Schedule ns;
    ns.enabled  = true;            // query already filtered enabled=true
    ns.portionG = s["portion_grams"] | DEFAULT_PORTION_G;
    ns.catId    = String((const char *)(s["cat_id"] | ""));
    ns.id       = String((const char *)(s["id"]     | ""));
    ns.lastFiredYday = 0;

    // Parse "HH:MM:SS" -> hour, minute.
    const char *tod = s["time_of_day"] | "00:00:00";
    int hh = 0, mm = 0;
    sscanf(tod, "%d:%d", &hh, &mm);
    ns.hour   = (uint8_t)constrain(hh, 0, 23);
    ns.minute = (uint8_t)constrain(mm, 0, 59);

    // Parse days_of_week int[] -> bitmask (bit d, 0=Sun .. 6=Sat).
    ns.daysMask = 0;
    JsonArray days = s["days_of_week"].as<JsonArray>();
    if (days.isNull() || days.size() == 0) {
      ns.daysMask = 0x7F; // every day by default
    } else {
      for (int d : days) ns.daysMask |= (1 << (d & 0x07));
    }

    scheduleAdd(ns);
  }
}

// --------------------------------------------------------------------------
// Called from STATE_IDLE once per loop. Iterates through the cached list
// and triggers at most one feed per schedule per day.
void checkScheduledFeeds() {
  if (!telemetry.rtcOk)              return;
  if (uiRequestedMode != UI_MODE_AUTO) return;  // only feed on schedule in AUTO mode
  if (scheduleCount == 0)           return;
  if (cycle.active)                 return;

  DateTime now = rtc.now();
  uint16_t yday = (uint16_t)now.year() * 400 +
                  (uint16_t)now.month() * 32 +
                  now.day();  // coarse unique day id
  uint8_t dow = now.dayOfTheWeek();   // 0=Sunday

  for (uint8_t i = 0; i < scheduleCount; i++) {
    Schedule& s = schedules[i];
    if (!s.enabled) continue;
    if (!((s.daysMask >> dow) & 0x01)) continue;
    if (now.hour() != s.hour || now.minute() != s.minute) continue;
    if (s.lastFiredYday == yday) continue;    // already fired today

    Serial.printf("[sched] firing schedule %u (%u:%02u, %.1f g)\n",
                  i, s.hour, s.minute, s.portionG);
    if (startDispense(s.portionG, "scheduled", String(""), s.catId, s.id)) {
      s.lastFiredYday = yday;
    }
    break;  // one at a time
  }
}
