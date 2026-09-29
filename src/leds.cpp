#include "leds.h"

CRGB leds[NUM_LEDS];  // Setting up the LED strip
static int8_t offset = LED_OFFSET_L;

void setLedOffset(uint8_t o) { offset = o; }
int8_t ledOffset() { return offset; }

CRGB& led(int i) { return leds[(i + offset) % NUM_LEDS]; }

CRGB grey(uint8_t v) { return CRGB(v, v, v); }

void fillAll(const CRGB& color) { fill_solid(leds, NUM_LEDS, color); }

void clearLEDs() { fillAll(CRGB::Black); }

bool blinkPhaseOff() { return millis() % BLINK_PERIOD_MS >= BLINK_PERIOD_MS / 2; }

void drawBlueRun(int count, uint8_t blue) {
  for (int i = 0; i < count; i++) led(i + 1).b = blue;
}

void drawMinuteAndSecond(DateTime now) {
  led(now.minute()).g = 255;
  led(now.second()).b = 255;
}

void drawHourSpread(int pos, bool solid) {
  led(pos - 1).r = 50;
  if (solid) led(pos) = CRGB::Red;
  else led(pos).r = 255;
  led(pos + 1).r = 50;
}

void drawHour24(uint8_t hour) {
  if (hour <= 11) led(hour * 5).r = 255;
  else drawHourSpread((hour - 12) * 5);
}

void hideHour(uint8_t hour) {
  int pos = (hour % 12) * 5;
  led(pos - 1).r = 0;
  led(pos).r = 0;
  led(pos + 1).r = 0;
}

void fillTicks(uint8_t bright) {
  for (int i = 0; i < NUM_LEDS; i += 5) led(i) = grey(bright);
}
