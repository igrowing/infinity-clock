// Pure clock logic: no Arduino, no hardware. Compiles for the Nano and for the native (PC) unit tests.
#pragma once
#include <stdint.h>

#define HOLD_TIME_MS 1500  // A press longer than this is a "long click"

// States of the clock
#define STATE_CLOCK 0
#define STATE_ALARM 1
#define STATE_SET_ALARM_HR 2
#define STATE_SET_ALARM_MIN 3
#define STATE_SET_CLOCK_HR 4
#define STATE_SET_CLOCK_MIN 5
#define STATE_SET_CLOCK_SEC 6
#define STATE_SET_CLOCK_UP 7
#define STATE_COUNTDOWN 8
#define STATE_DEMO 9

// Add `delta` to `value` and wrap into 0..mod-1.
int wrapStep(int value, int delta, int mod);

// New countdown time in seconds after the rotary moved by `move` minutes.
// Minutes form a ring of 61 values (0..60), so 0 -1-> 60 -+1-> 0.
long countdownAfterRotate(long currentCountDown, int move);

// State to enter when the menu button is released while the clock shows the time (STATE_CLOCK).
// `pressSeen`: a press was actually observed before this release (otherwise the release is ignored).
// `pressedMs`: how long that press lasted.
int stateOnMenuRelease(bool pressSeen, long pressedMs);

// Index of the LED reached by the alarm ramp-up animation, `elapsedMs` after the alarm fired (one LED per 300 ms).
// Saturates at INT16_MAX instead of wrapping.
int16_t alarmRampPosition(uint32_t elapsedMs);
