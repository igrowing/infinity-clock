#include "faces.h"
#include "leds.h"
#include <clock_logic.h>

// ---- Measuring the second. The RTC only tells the whole seconds; the faces that move within a second
// need to know how long a second lasts on this loop, and where in it we are.
static DateTime lastSecond;        // The time when the current second was noticed
static long newSecTime = 0;        // millis() at that moment
static int cyclesPerSec = 0;       // Length of the previous second in ms (0 until measured)
static float cyclesPerSecFloat = 0;

// True when a new second has just started. Also measures the length of the previous second.
static bool startOfNewSecond(DateTime now) {
  if (now.second() == lastSecond.second()) return false;
  lastSecond = now;
  cyclesPerSec = millis() - newSecTime;
  cyclesPerSecFloat = (float) cyclesPerSec;
  newSecTime = millis();
  return true;
}

static bool secondLengthKnown() { return cyclesPerSec > 0; }

// How far we are into the current second: 0.0 .. 1.0
static float secondProgress() { return (millis() - newSecTime) / cyclesPerSecFloat; }

void restartSecondTimer() { newSecTime = millis(); }

// ---- The faces

// Just 3 LEDs on.
static void minimalClock(DateTime now) {
  unsigned char hourPos = (now.hour()%12)*5;
  led(hourPos).r = 255;
  drawMinuteAndSecond(now);
}

// # Red LEDs for hours, 1 LED per min and sec.
static void basicClock(DateTime now) {
  drawHourSpread(hourHandPosition(now.hour(), now.minute()), true);
  // Mix colors if the same LED is chosen
  drawMinuteAndSecond(now);
}

// Second hand flowing from sec-to-sec + Basic clock
static void smoothSecond(DateTime now) {
  static int brightness = 0;  // Kept from frame to frame while the second length is unknown
  basicClock(now);
  startOfNewSecond(now);
  // set hour, min & sec LEDs
  float fracOfSec = secondProgress();
  if (secondLengthKnown()) { brightness = 50.0*(1.0+sin((PI*fracOfSec)-HALF_PI)); }
  led(now.second()).b = brightness;
  led(now.second()-1).b = 100 - brightness;
}

// Constant lit 5-minute ticks + Basic clock
static void outlineClock(DateTime now) {
  fillTicks(100);
  basicClock(now);
}

static void starryNightClock(DateTime now) {
  static uint8_t star = 0;
  static float starBlinks = 0;
  basicClock(now);
  if (startOfNewSecond(now)) {
    star = random8(NUM_LEDS);           // Choose star
    starBlinks = (float)(random8(1, 4) * 8);   // Choose times of sparkles
  }
  float m = (float) (millis() % 2000) / 3000.0;
  int brightness = (2.0-m)*15.0*(1.0+sin(m*starBlinks-0.7));
  brightness = min(brightness, 100);  // cut numbers > 100
  brightness = (brightness < 15)?0:brightness;  // cut numbers < 15
  leds[star] = grey(brightness);
}

// Running white light over clock round. Full round in 1 second.
static void minimalMilliSec(DateTime now) {
  startOfNewSecond(now);
  // Millisec lights are set first, so hour/min/sec lights override and don't flicker as millisec passes
  if (secondLengthKnown()) {
    int subSeconds = (((millis() - newSecTime)*60)/cyclesPerSec)%60;  // 60th's of a second
    led(subSeconds) = grey(50);
  }
  // The colours are set last, so if on same LED mixed colours are created
  drawHourSpread(hourHandPosition(now.hour(), now.minute()));
  drawMinuteAndSecond(now);
}

// Pendulum will be at the bottom and left for one second and right for one second
static void simplePendulum(DateTime now) {
  static float swing = HALF_PI;  // Direction of the swing in this second
  static int pendulumPos = 0;    // Kept from frame to frame while the second length is unknown
  basicClock(now);
  if (startOfNewSecond(now)) swing = -swing;
  float fracOfSec = secondProgress();
  if (secondLengthKnown()) {
    pendulumPos = pendulumLed(fracOfSec, swing, ledOffset());
  }
  // Pendulum lights are set last, on top of the hour/min/sec lights
  leds[pendulumPos] = CRGB::WhiteSmoke;
}

static void breathingClock(DateTime now) {
  int brightness = 30.0*(1.0+sin((PI*millis()/2000.0)-HALF_PI)) + 2;
  fillTicks(brightness);
  basicClock(now);
}

// ---- The list of faces. The rotary selects them by their number here.
typedef void (*Face)(DateTime now);
static const Face FACES[] = {minimalClock, basicClock, smoothSecond, outlineClock,
                             minimalMilliSec, simplePendulum, breathingClock, starryNightClock};

uint8_t clockFaceCount() { return sizeof(FACES)/sizeof(FACES[0]); }

bool drawClockFace(uint8_t mode, DateTime now) {
  if (mode >= clockFaceCount()) return false;
  FACES[mode](now);
  return true;
}
