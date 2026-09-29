#include "settings.h"
#include "config.h"
#include "faces.h"
#include "log.h"
#include "rtc.h"

void loadSettings(MenuModel& m) {
  // Uncomment to reset all the NVRAM addresses. You will have to comment again and reload, otherwise it will not save anything each time power is cycled
  // write a 0 to all 512 uint8_ts of the NVRAM
//  for (int i = 0; i < 512; i++)
//  {RTC.writenvram(i, 0);}

  Settings saved = sanitizeSettings({RTC.readnvram(CLOCK_MODE_ADDR), RTC.readnvram(ALARM_MIN_ADDR),
                                     RTC.readnvram(ALARM_HR_ADDR), RTC.readnvram(ALARM_SET_ADDR),
                                     RTC.readnvram(ALARM_MODE_ADDR), RTC.readnvram(LED_OFFSET_ADDR)},
                                    clockFaceCount());
  m.clockMode = saved.clockMode;
  m.alarmMin = saved.alarmMin;
  m.alarmHour = saved.alarmHour;
  m.alarmSet = saved.alarmSet;
  m.alarmMode = saved.alarmMode;
  m.ledOffset = saved.ledOffset;

  // Write sanitized data back to NVRAM for further proper boot.
  RTC.writenvram(CLOCK_MODE_ADDR, m.clockMode);
  RTC.writenvram(ALARM_MIN_ADDR, m.alarmMin);
  RTC.writenvram(ALARM_HR_ADDR, m.alarmHour);
  RTC.writenvram(ALARM_SET_ADDR, m.alarmSet);
  RTC.writenvram(ALARM_MODE_ADDR, m.alarmMode);
  RTC.writenvram(LED_OFFSET_ADDR, m.ledOffset);
}

void saveSettings(const MenuModel& m, const MenuEffects& fx) {
  if (fx.saveClockMode) RTC.writenvram(CLOCK_MODE_ADDR, m.clockMode);
  if (fx.saveAlarmSet) RTC.writenvram(ALARM_SET_ADDR, m.alarmSet);
  if (fx.saveAlarmMode) RTC.writenvram(ALARM_MODE_ADDR, m.alarmMode);
  if (fx.saveAlarmHour) RTC.writenvram(ALARM_HR_ADDR, m.alarmHour);
  if (fx.saveAlarmMin) RTC.writenvram(ALARM_MIN_ADDR, m.alarmMin);
  if (fx.saveLedOffset) RTC.writenvram(LED_OFFSET_ADDR, m.ledOffset);
}

void printSettings(const MenuModel& m) {
  printValue(F("Mode is "), m.clockMode);
  printValue(F("Alarm Hour is "), m.alarmHour);
  printValue(F("Alarm Min is "), m.alarmMin);
  printValue(F("Alarm is set "), m.alarmSet);
  printValue(F("Alarm Mode is "), m.alarmMode);
  printValue(F("LED offset is "), m.ledOffset);
}
