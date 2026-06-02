#ifndef SCHEDULES_H
#define SCHEDULES_H

// ==========================================================================
// schedules.h — shared schedule type
// ==========================================================================
// Defined in a header (not an .ino tab) so the Schedule type is visible to
// every tab BEFORE the Arduino IDE hoists auto-generated prototypes such as
// scheduleAdd(const Schedule&) to the top of the combined sketch.
// ==========================================================================

#include <Arduino.h>

#define MAX_SCHEDULES 20

struct Schedule {
  bool     enabled;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  daysMask;       // bit0 = Sunday ... bit6 = Saturday; 0x7F = every day
  float    portionG;
  String   catId;
  String   id;
  uint16_t lastFiredYday;  // unique day id of the last firing (anti double-fire)
};

#endif // SCHEDULES_H
