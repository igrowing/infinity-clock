#include "clock_logic.h"

int wrapStep(int value, int delta, int mod) {
  return (value + delta < 0) ? mod - 1 : (value + delta) % mod;
}

long countdownAfterRotate(long currentCountDown, int move) {
  // Convert currentCountDown secs to mins, add change, wrap in 0..60 mins, convert back to secs.
  return (long)wrapStep(currentCountDown/60, move, 61) * 60;
}

int16_t alarmRampPosition(uint32_t elapsedMs) {
  uint32_t pos = elapsedMs/300;
  return pos > INT16_MAX ? INT16_MAX : (int16_t)pos;
}
