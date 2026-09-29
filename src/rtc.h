// The real time clock: one instance, defined in main.ino.
#pragma once
#include <RTClib.h>
#include <menu_machine.h>

extern RTC_DS1307 RTC;

// The time as the (hardware independent) menu logic wants it.
inline NowInfo toNowInfo(const DateTime& t) {
  NowInfo n = {};
  n.year = t.year();
  n.month = t.month();
  n.day = t.day();
  n.hour = t.hour();
  n.minute = t.minute();
  n.second = t.second();
  n.unixtime = t.unixtime();
  return n;
}
