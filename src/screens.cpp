#include "screens.h"
#include "leds.h"
#include "faces.h"
#include "buzzer.h"
#include "ticker.h"
#include <clock_logic.h>

// ---- Setting screens

static void setAlarmDisplay(const MenuModel& m) {
  // Background: 5-minute ticks. Red = alarm is off, green = alarm is on.
  CRGB tick = m.alarmSet ? CRGB(0, 20, 0) : CRGB(20, 0, 0);
  for (int i = 0; i < NUM_LEDS; i += 5) led(i) = tick;
  drawHour24(m.alarmHour);
  led(m.alarmMin).g = 100;
  // Turn off the value being set periodically to show flashing.
  bool off = blinkPhaseOff();
  if (m.state == STATE_SET_ALARM_HR && off) hideHour(m.alarmHour);
  if (m.state == STATE_SET_ALARM_MIN && off) led(m.alarmMin).g = 0;
  led(m.alarmMode).b = 255;
}

static void setClockDisplay(const MenuModel& m, DateTime now) {
  fillTicks(10);
  drawHour24(now.hour());
  bool off = blinkPhaseOff();
  if (m.state == STATE_SET_CLOCK_HR && off) hideHour(now.hour());
  if (m.state == STATE_SET_CLOCK_MIN && off) led(now.minute()).g = 0;
  else led(now.minute()).g = 255;
  if (m.state == STATE_SET_CLOCK_SEC && off) led(now.second()).b = 0;
  else led(now.second()).b = 255;
}

// ---- Alarm effects

static uint32_t alarmRingingSince = 0;   // millis() when the alarm started to ring

void alarmStarted() { alarmRingingSince = millis(); }

// Dim white on all LEDs: avoid overcurrent.
static void alarmSolid() {
  fillAll(grey(20));
}

// White grows from both ends of the ring, then fills everything.
static void alarmRamp() {
  int16_t pos = alarmRampPosition(millis() - alarmRingingSince);
  int16_t reversePos = NUM_LEDS - pos;
  if (pos >= 0 && pos <= (NUM_LEDS/2-1)) {
    for (int i = 0; i < pos; i++) led(i) = grey(5);
  } else {
    fillAll(grey(5));
  }
  if (reversePos <= (NUM_LEDS-1) && reversePos >= (NUM_LEDS/2+1)) {
    for (int i = NUM_LEDS-1; i > reversePos; i--) led(i) = grey(5);
  }
}

// Brightness fades up over FADE_TIME_MS.
static void alarmFade() {
  fillAll(grey(alarmFadeBrightness(millis() - alarmRingingSince)));
}

// Alarm effects, selected by alarmMode 1..ALARM_MODE_MAX (mode 0 is "alarm off").
typedef void (*AlarmEffect)();
static const AlarmEffect ALARM_EFFECTS[] = {alarmSolid, alarmRamp, alarmFade};
static_assert(sizeof(ALARM_EFFECTS)/sizeof(ALARM_EFFECTS[0]) == ALARM_MODE_MAX, "ALARM_MODE_MAX must match ALARM_EFFECTS");

void alarmDisplay(uint8_t alarmMode) {
  runBuzzer(BUZZER_ALARM);
  if (alarmMode >= 1 && alarmMode <= ALARM_MODE_MAX) ALARM_EFFECTS[alarmMode - 1]();
}

// ---- Timer

// The idle animation of the timer screen: a blue line growing round the ring and shrinking back.
static struct {
  int pos = 0;
  bool growing = true;
  Ticker ticker;
} sweep;

void resetSweep() { sweep.pos = 0; }

static void drawSweep() {
  drawBlueRun(sweep.pos, 20);
  if (sweep.ticker.elapsed(TIME_INTERVAL)) sweep.pos += sweep.growing ? 1 : -1;
  if (sweep.growing && sweep.pos == NUM_LEDS) sweep.growing = false;
  else if (!sweep.growing && sweep.pos < 0) sweep.growing = true;
}

static void countDownDisplay(MenuModel& m, DateTime now) {
  if (m.countDown) {
    // Counting down
    m.currentCountDown = countdownRemaining(m.countDownTime, m.startCountDown, now.unixtime());
    if (m.currentCountDown > 0) {
      // Time is not gone yet, decrement lights
      int minutes = m.currentCountDown / 60;
      int seconds = m.currentCountDown%60 * 2; // Range 0-120 to create brightness less than 240
      drawBlueRun(minutes, 240); // A blue LED for each complete minute that is remaining
      led(minutes+1).b = seconds; // Display the remaining secconds of the current minute as its brightness
    } else {
      // Time is gone, flash and beep.
      runBuzzer(BUZZER_TIMER);
      if (now.unixtime()%2 == 0) clearLEDs();
      else fillAll(CRGB::Blue);  // Set the background as all blue
    }
  } else {
    // Setting mode
    m.currentCountDown = m.countDownTime;
    if (m.countDownTime == 0) {
      clearLEDs();
      drawSweep();
    } else if (m.countDownTime > 0 && blinkPhaseOff()) {
      drawBlueRun(m.currentCountDown / 60, 255); // A blue LED for each complete minute that is remaining
    }
  }
}

// ---- Demo: the colours wipe over the ring one by one, then a rainbow

static const CRGB DEMO_COLORS[9] = {CRGB::Black, CRGB::Red, CRGB::Red, CRGB::Green, CRGB::Green, CRGB::Blue, CRGB::Blue, CRGB::White, CRGB::White};
#define DEMO_RAINBOW_STEP 9           // Steps 1..8 are the colour wipes
#define DEMO_RAINBOW_MS (TIME_INTERVAL * 5000UL)

static struct {
  uint8_t step = 0;          // 0 = show the clock, 1..8 = colour wipes, 9 = rainbow
  int pos = 0;               // How far the current wipe has got
  uint8_t rainbowShift = 0;  // Where the rainbow starts
  Ticker wipeTicker;
  Ticker rainbowTicker;
} demo;

void startDemo() {
  demo.step = 1;
  demo.pos = 0;
}

// Odd steps paint the colour in, even steps wipe it out again.
static void colorWipe() {
  const CRGB& color = DEMO_COLORS[demo.step];
  if (demo.step % 2 == 1) {
    for (int i = 0; i < demo.pos; i++) led(i + 1) = color;
  } else {
    for (int i = demo.pos; i < NUM_LEDS; i++) led(i + 1) = color;
  }
  if (demo.wipeTicker.elapsed(TIME_INTERVAL)) demo.pos++;
  if (demo.pos == NUM_LEDS) {
    demo.pos = 0;
    demo.step++;
    if (demo.step == DEMO_RAINBOW_STEP) demo.rainbowTicker.restart();
  }
}

static void rainbow() {
  const uint8_t R_SHIFT = 0, G_SHIFT = 20, B_SHIFT = 40;
  static const uint8_t color_intensity[] = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 5,
                                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                            5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100};
  // Progress the initial position for the rainbow.
  demo.rainbowShift = (demo.rainbowShift + 1) % NUM_LEDS;

  // Fill the rainbow
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].r = color_intensity[(demo.rainbowShift + i + R_SHIFT) % NUM_LEDS];
    leds[i].g = color_intensity[(demo.rainbowShift + i + G_SHIFT) % NUM_LEDS];
    leds[i].b = color_intensity[(demo.rainbowShift + i + B_SHIFT) % NUM_LEDS];
  }
  delay(10);
}

static void runDemo(uint8_t clockMode, DateTime now) {
  clearLEDs();
  if (demo.step == 0) {
    drawClockFace(clockMode, now);
  } else if (demo.step < DEMO_RAINBOW_STEP) {
    colorWipe();
  } else {
    rainbow();
    if (demo.rainbowTicker.elapsed(DEMO_RAINBOW_MS)) startDemo();  // Start over
  }
}

// ---- Which screen is on

bool renderScreen(MenuModel& m, DateTime now) {
  switch (m.state) {
    case STATE_SET_CLOCK_HR:
    case STATE_SET_CLOCK_MIN:
    case STATE_SET_CLOCK_SEC:
    case STATE_SET_CLOCK_UP:
      setClockDisplay(m, now);
      return true;
    case STATE_ALARM:
    case STATE_SET_ALARM_HR:
    case STATE_SET_ALARM_MIN:
      setAlarmDisplay(m);
      return true;
    case STATE_COUNTDOWN:
      countDownDisplay(m, now);
      return true;
    case STATE_DEMO:
      runDemo(m.clockMode, now);
      return true;
    case STATE_CLOCK:
    default:
      return drawClockFace(m.clockMode, now);
  }
}
