// Pure clock logic: no Arduino, no hardware. Compiles for the Nano and for the native (PC) unit tests.
#pragma once
#include <stdint.h>

#define NUM_LEDS    60    // Number of LEDs in strip
#define HOLD_TIME_MS 1500  // A press longer than this is a "long click"
#define FADE_TIME_MS 60000 // Alarm mode 3: time to fade up from dark to full brightness

// States of the clock
enum State : uint8_t {
  STATE_CLOCK,
  STATE_ALARM,
  STATE_SET_ALARM_HR,
  STATE_SET_ALARM_MIN,
  STATE_SET_CLOCK_HR,
  STATE_SET_CLOCK_MIN,
  STATE_SET_CLOCK_SEC,
  STATE_SET_CLOCK_UP,
  STATE_COUNTDOWN,
  STATE_DEMO
};

#define ALARM_MODE_MAX 3   // Alarm modes are 0 (alarm off) .. ALARM_MODE_MAX
#define LED_OFFSET_L 52    // Adjust by LED position/shift in the circle
#define LED_OFFSET_R 37    // Adjust by LED position/shift in the circle

// Settings kept in the RTC NVRAM. Raw bytes as read from the NVRAM (virgin NVRAM holds 0x00 or 0xFF).
struct Settings {
  uint8_t clockMode;
  uint8_t alarmMin;
  uint8_t alarmHour;
  uint8_t alarmSet;   // 0 or 1
  uint8_t alarmMode;
  uint8_t ledOffset;  // LED_OFFSET_L or LED_OFFSET_R
};

// Replace every out-of-range field with its default. `clockModeCount` is the number of clock faces.
Settings sanitizeSettings(Settings s, uint8_t clockModeCount);

// Add `delta` to `value` and wrap into 0..mod-1.
int wrapStep(int value, int delta, int mod);

// New countdown time in seconds after the rotary moved by `move` minutes.
// Minutes form a ring of 61 values (0..60), so 0 -1-> 60 -+1-> 0.
long countdownAfterRotate(long currentCountDown, int move);

// State to enter when the menu button is released while the clock shows the time (STATE_CLOCK).
// `pressSeen`: a press was actually observed before this release (otherwise the release is ignored).
// `pressedMs`: how long that press lasted.
State stateOnMenuRelease(bool pressSeen, long pressedMs);

// Index of the LED reached by the alarm ramp-up animation, `elapsedMs` after the alarm fired (one LED per 300 ms).
// Saturates at INT16_MAX instead of wrapping.
int16_t alarmRampPosition(uint32_t elapsedMs);

// Brightness (0..255) of the fade-up alarm, `elapsedMs` after the alarm fired.
uint8_t alarmFadeBrightness(uint32_t elapsedMs);

// Strip index of the pendulum LED. `fracOfSec` is how far into the current second we are (0..1); `swing` is
// +HALF_PI or -HALF_PI and selects the swing direction of this second. The pendulum hangs at the bottom of
// the clock face (position 30) and swings 3 LEDs to each side.
int pendulumLed(float fracOfSec, float swing, int ledOffset);

// LED position of the hour hand on the clock face: 5 LEDs per hour, moving on with the minutes.
// Can be NUM_LEDS just before the top of the hour; the strip wraps it.
int hourHandPosition(uint8_t hour, uint8_t minute);

// Seconds left on a timer of `lengthSec` started at unix time `startedAt`. Zero or negative once it has run out.
long countdownRemaining(long lengthSec, long startedAt, uint32_t nowUnix);
