// Pure clock logic: no Arduino, no hardware. Compiles for the Nano and for the native (PC) unit tests.
#pragma once
#include <stdint.h>

// Add `delta` to `value` and wrap into 0..mod-1.
int wrapStep(int value, int delta, int mod);

// New countdown time in seconds after the rotary moved by `move` minutes.
// Minutes form a ring of 61 values (0..60), so 0 -1-> 60 -+1-> 0.
long countdownAfterRotate(long currentCountDown, int move);

// Index of the LED reached by the alarm ramp-up animation, `elapsedMs` after the alarm fired (one LED per 300 ms).
// Saturates at INT16_MAX instead of wrapping.
int16_t alarmRampPosition(uint32_t elapsedMs);
