// The LED ring and the drawing primitives every clock face and screen is made of.
// Positions are on the clock face: 0 is 12 o'clock, counted clockwise, 60 LEDs.
#pragma once
#include <FastLED.h>
#include <RTClib.h>
#include "config.h"

extern CRGB leds[NUM_LEDS];  // Raw strip. Prefer led(): it applies the mounting offset.

// Where 12 o'clock is on the strip (LED_OFFSET_L or LED_OFFSET_R).
void setLedOffset(uint8_t offset);
int8_t ledOffset();

// LED at position `i` of the clock face, taking the mounting offset into account.
CRGB& led(int i);

CRGB grey(uint8_t v);
void fillAll(const CRGB& color);
void clearLEDs();

// True in the "off" half of the flashing period. Used to flash the value being set.
bool blinkPhaseOff();

// A blue LED at every position from 1 to `count`.
void drawBlueRun(int count, uint8_t blue);

// Minute (green) and second (blue) hands.
void drawMinuteAndSecond(DateTime now);

// Hour hand of the afternoon: red centre LED with dim neighbours.
// `solid`: the centre LED is pure red instead of just having its red channel raised.
void drawHourSpread(int pos, bool solid = false);

// Hour hand for a 24h `hour`: a single LED before noon, a 3-LED spread after noon.
void drawHour24(uint8_t hour);

// Turn the hour hand off (the "flash" phase while setting the hour).
void hideHour(uint8_t hour);

// Dim marks at every 5 minutes.
void fillTicks(uint8_t bright);
