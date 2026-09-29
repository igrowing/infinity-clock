#include "clock_logic.h"
#include <math.h>

static const float HALF_PI_F = 1.5707963f;

int wrapStep(int value, int delta, int mod) {
  return (value + delta < 0) ? mod - 1 : (value + delta) % mod;
}

long countdownAfterRotate(long currentCountDown, int move) {
  // Convert currentCountDown secs to mins, add change, wrap in 0..60 mins, convert back to secs.
  return (long)wrapStep(currentCountDown/60, move, 61) * 60;
}

Settings sanitizeSettings(Settings s, uint8_t clockModeCount) {
  if (s.clockMode >= clockModeCount) s.clockMode = 0;
  if (s.alarmMin >= 60) s.alarmMin = 0;
  if (s.alarmHour >= 24) s.alarmHour = 0;
  if (s.alarmSet > 1) s.alarmSet = 0;
  if (s.alarmMode > ALARM_MODE_MAX) s.alarmMode = 0;
  if (s.ledOffset != LED_OFFSET_L && s.ledOffset != LED_OFFSET_R) s.ledOffset = LED_OFFSET_L;
  return s;
}

State stateOnMenuRelease(bool pressSeen, long pressedMs) {
  if (!pressSeen) return STATE_CLOCK;  // Spurious release (e.g. pin level settling at boot): ignore.
  return (pressedMs <= HOLD_TIME_MS) ? STATE_ALARM : STATE_SET_CLOCK_HR;
}

int16_t alarmRampPosition(uint32_t elapsedMs) {
  uint32_t pos = elapsedMs/300;
  return pos > INT16_MAX ? INT16_MAX : (int16_t)pos;
}

uint8_t alarmFadeBrightness(uint32_t elapsedMs) {
  if (elapsedMs > FADE_TIME_MS) return 255;
  float fadeRad = (float)elapsedMs / FADE_TIME_MS;  // Fraction of the fade-up period, 0.0 .. 1.0
  return (int)(255.0 * (1.0 + sin(HALF_PI_F * fadeRad - HALF_PI_F)));
}
