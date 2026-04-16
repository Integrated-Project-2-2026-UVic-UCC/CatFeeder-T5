// ==========================================================================
// RTCManager.ino — DS3231 real-time clock + schedule evaluator
// ==========================================================================
// Schedules live in memory here; they are refreshed from device_config in
// Network.ino (pollDeviceConfig). Everything is checked locally so feeding
// keeps working when the network is down (SRS §REQ-NF-10).
// ==========================================================================

#define MAX_SCHEDULES 16

struct Schedule {
  bool     enabled;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  daysMask;       // bit 0 = Sunday ... bit 6 = Saturday; 0x7F = every day
  float    portionG;
  String   catId;
  String   id;
  uint16_t lastFiredYday;  // day-of-year of the last firing (avoids double-fire)
};

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
// Called from STATE_IDLE once per loop. Iterates through the cached list
// and triggers at most one feed per schedule per day.
void checkScheduledFeeds() {
  if (!telemetry.rtcOk)        return;
  if (scheduleCount == 0)      return;
  if (cycle.active)            return;

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
