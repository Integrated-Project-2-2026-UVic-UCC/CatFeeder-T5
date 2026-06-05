// ==========================================================================
// RTCManager.ino — Schedule manager (RTC-free)
// ==========================================================================
// The DS3231 RTC has been removed to avoid I2C bus conflicts with the
// ILI9341 touch display (XPT2046). Scheduled feeding by time-of-day is
// therefore disabled; the webapp can still trigger manual dispenses via
// the commands table.
//
// The schedule data structures and loadSchedules() are kept intact so that
// Network.ino can parse the Supabase `schedules` array without modification.
// checkScheduledFeeds() is a no-op until an NTP-based clock is integrated.
// ==========================================================================

#include <ArduinoJson.h>
#include <time.h>
#include "schedules.h"   // Schedule struct + MAX_SCHEDULES

// Forward declarations
bool startDispense(float grams, const char *trigger, const String &commandId, const String &catId, const String &scheduleId);

DeviceTime currentDeviceTime = {false, 0, 0, 0, 0, 0};

static uint32_t lastTimeUpdateMs = 0;
static time_t lastLocalEpoch = 0;

// Dependency-free UTC to epoch converter (standard substitute for timegm)
static time_t my_timegm(const struct tm *tm) {
  int year = tm->tm_year + 1900;
  int month = tm->tm_mon + 1; // 1-12
  int day = tm->tm_mday;
  int hour = tm->tm_hour;
  int min = tm->tm_min;
  int sec = tm->tm_sec;

  if (month < 3) {
    year -= 1;
    month += 12;
  }
  
  long days = (365 * year) + (year / 4) - (year / 100) + (year / 400) + ((153 * month - 457) / 5) + day - 719469;
  return (days * 86400) + (hour * 3600) + (min * 60) + sec;
}

Schedule schedules[MAX_SCHEDULES];
uint8_t  scheduleCount = 0;

// --------------------------------------------------------------------------
// scheduleClear — clears the in-memory schedule list.
// Called before re-hydrating from device_config.
void scheduleClear() {
  scheduleCount = 0;
}

// --------------------------------------------------------------------------
// scheduleAdd — appends one schedule entry (called from loadSchedules()).
void scheduleAdd(const Schedule& s) {
  if (scheduleCount >= MAX_SCHEDULES) return;
  schedules[scheduleCount++] = s;
}

// --------------------------------------------------------------------------
// updateLocalClock — increments our software clock based on elapsed millis()
void updateLocalClock() {
  if (!currentDeviceTime.valid) return;
  
  uint32_t elapsedSec = (millis() - lastTimeUpdateMs) / 1000;
  time_t currentEpoch = lastLocalEpoch + elapsedSec;
  
  struct tm localTm;
  gmtime_r(&currentEpoch, &localTm);
  
  currentDeviceTime.hour = localTm.tm_hour;
  currentDeviceTime.minute = localTm.tm_min;
  currentDeviceTime.second = localTm.tm_sec;
  currentDeviceTime.wday = localTm.tm_wday;
  currentDeviceTime.yday = localTm.tm_yday;
}

// --------------------------------------------------------------------------
// updateTimeFromSupabase — syncs our software clock with Supabase UTC time
void updateTimeFromSupabase(const char* utcStr) {
  int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
  if (sscanf(utcStr, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6 ||
      sscanf(utcStr, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
    
    struct tm t;
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    t.tm_isdst = -1;
    
    time_t utcEpoch = my_timegm(&t);
    lastLocalEpoch = utcEpoch + TIMEZONE_OFFSET_SEC;
    lastTimeUpdateMs = millis();
    currentDeviceTime.valid = true;
    
    updateLocalClock();
    
    Serial.printf("[time] Synced: %02d:%02d:%02d (wday=%d, yday=%d)\n",
                  currentDeviceTime.hour, currentDeviceTime.minute, currentDeviceTime.second,
                  currentDeviceTime.wday, currentDeviceTime.yday);
  }
}

// --------------------------------------------------------------------------
// loadSchedules — rehydrates the in-memory list from a Supabase
// `schedules` array:
//   [ { "id":.., "cat_id":.., "time_of_day":"HH:MM:SS",
//       "days_of_week":[0..6], "portion_grams":25 }, ... ]
void loadSchedules(JsonArray& arr) {
  // Store old schedules temporarily to preserve their lastFiredYday anti-double-fire flags
  Schedule oldSchedules[MAX_SCHEDULES];
  uint8_t oldCount = scheduleCount;
  for (uint8_t i = 0; i < oldCount; i++) {
    oldSchedules[i] = schedules[i];
  }

  scheduleClear();
  for (JsonObject s : arr) {
    if (scheduleCount >= MAX_SCHEDULES) break;

    Schedule ns;
    ns.enabled  = true;            // query already filtered enabled=true
    ns.portionG = s["portion_grams"] | DEFAULT_PORTION_G;
    ns.catId    = String((const char *)(s["cat_id"] | ""));
    ns.id       = String((const char *)(s["id"]     | ""));
    
    // Look up and preserve lastFiredYday if the schedule was already loaded
    ns.lastFiredYday = 999; // 999 is a safe sentinel indicating not fired today
    for (uint8_t i = 0; i < oldCount; i++) {
      if (oldSchedules[i].id == ns.id) {
        ns.lastFiredYday = oldSchedules[i].lastFiredYday;
        break;
      }
    }

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
  Serial.printf("[sched] %u schedule(s) loaded (time-based firing active via Supabase)\n",
                scheduleCount);
}

// --------------------------------------------------------------------------
// checkScheduledFeeds — monitors local time against loaded schedules
void checkScheduledFeeds() {
  if (!currentDeviceTime.valid) return;
  
  updateLocalClock();
  
  for (uint8_t i = 0; i < scheduleCount; i++) {
    Schedule &s = schedules[i];
    if (!s.enabled) continue;
    
    // Check day of week (s.daysMask bit d, 0=Sun .. 6=Sat)
    if ((s.daysMask & (1 << currentDeviceTime.wday)) == 0) continue;
    
    // Check hour and minute
    if (currentDeviceTime.hour == s.hour && currentDeviceTime.minute == s.minute) {
      if (s.lastFiredYday != currentDeviceTime.yday) {
        // Trigger dispense; only mark as fired if successfully started
        if (startDispense(s.portionG, "scheduled", String(""), s.catId, s.id)) {
          s.lastFiredYday = currentDeviceTime.yday;
          Serial.printf("[sched] triggered schedule %s (%.1fg)\n", s.id.c_str(), s.portionG);
          break; // trigger at most one schedule per check loop
        }
      }
    }
  }
}
