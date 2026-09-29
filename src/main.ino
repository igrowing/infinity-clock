/***********************************************************
 * Inspired by http://barkengmad.com/rise-and-shine-led-clock/
 * and based Morgan Barke's code.
 * 
 * Modded by iGrowing 2019.
 * https://github.com/igrowing/infinity-clock
 ***********************************************************/

//Add the following libraries to the respective folder for you operating system. See http://arduino.cc/en/Guide/Environment
#include <FastLED.h> // FastSPI Library version 3.1.X from https://github.com/FastLED/FastLED. Version 3.0.X has a bug: all LEDs are green by default.
#include <Wire.h> //This is to communicate via I2C. On arduino Uno & Nano use pins A4 for SDA (yellow/orange) and A5 for SCL (green). For other boards ee http://arduino.cc/en/Reference/Wire
#include <clock_logic.h>  // Pure logic, unit-tested on PC (see test/)
#include <menu_machine.h>  // Menu state machine, unit-tested on PC
#include <buzzer_sequencer.h>  // Beep timing, unit-tested on PC
#include <RTClib.h>           // Include the RTClib library to enable communication with the real time clock.
#include <Bounce2.h>          // Include the Bounce library for de-bouncing issues with push buttons.
#include <Encoder.h>          // Include the Encoder library to read the out puts of the rotary encoders

RTC_DS1307 RTC;     // Establishes the chipset of the Real Time Clock

#define TIMER       1      // Modes of buzzer
#define ALARM       2
#define BEEP_TONE   700    // Hz
#define PIN_BUZZER  9      // OC1A (Timer1 hardware output). Must be D9 for the hardware tone.
#define PIN_LEDS    A0
#define PIN_MENU    PIN4
#define DEMO_TIME_S 12 // seconds
#define ROTARY_SET_TIME_MS 300
#define TIME_INTERVAL 5
#define BLINK_PERIOD_MS 300  // Flashing while a value is being set: half of it off, half on

CRGB leds[NUM_LEDS];  // Setting up the LED strip
const CRGB DEMO_COLORS[9] = {CRGB::Black, CRGB::Red, CRGB::Red, CRGB::Green, CRGB::Green, CRGB::Blue, CRGB::Blue, CRGB::White, CRGB::White};
Encoder rotary1(PIN2, PIN3); // Setting up the Rotary Encoder

// Clock faces. To add a new one: write `void myFace(DateTime now)` and append it to FACES.
void minimalClock(DateTime now);
void basicClock(DateTime now);
void smoothSecond(DateTime now);
void outlineClock(DateTime now);
void minimalMilliSec(DateTime now);
void simplePendulum(DateTime now);
void breathingClock(DateTime now);
void starryNightClock(DateTime now);
typedef void (*Face)(DateTime now);
const Face FACES[] = {minimalClock, basicClock, smoothSecond, outlineClock,
                      minimalMilliSec, simplePendulum, breathingClock, starryNightClock};
#define CLOCK_MODE_MAX (sizeof(FACES)/sizeof(FACES[0]))  // Number of faces; the rotary wraps around it.

// Alarm effects, selected by alarmMode 1..ALARM_MODE_MAX (mode 0 is "alarm off").
void alarmSolid();
void alarmRamp();
void alarmFade();
typedef void (*AlarmEffect)();
const AlarmEffect ALARM_EFFECTS[] = {alarmSolid, alarmRamp, alarmFade};
static_assert(sizeof(ALARM_EFFECTS)/sizeof(ALARM_EFFECTS[0]) == ALARM_MODE_MAX, "ALARM_MODE_MAX must match ALARM_EFFECTS");

DateTime old; // Variable to compare new and old time, to see if it has moved on.
int rotary1Pos = 0;
int subSeconds; // 60th's of a second
int brightness = 0;
long newSecTime; // Variable to record when a new second starts, allowing to create milli seconds
long breathCycleTime;
int cyclesPerSec;
float cyclesPerSecFloat;
float fracOfSec;
float breathFracOfSec;
boolean demo;
long previousDemoTime;
long currentDemoTime;
float swingBack = HALF_PI;

volatile uint8_t alarmMin; // The minute of the alarm  
volatile uint8_t alarmHour; // The hour of the alarm 0-23
volatile uint8_t alarmDay = 0; // The day of the alarm
volatile boolean alarmSet; // Whether the alarm is set or not
#define CLOCK_MODE_ADDR  0 // Address of where mode is stored in the NVRAM
#define ALARM_MIN_ADDR   1 // Address of where alarm minute is stored in the NVRAM
#define ALARM_HR_ADDR    2 // Address of where alarm hour is stored in the NVRAM
#define ALARM_SET_ADDR   3 // Address of where alarm state is stored in the NVRAM
#define ALARM_MODE_ADDR  4 // Address of where the alarm mode is stored in the NVRAM
#define LED_OFFSET_ADDR  5 // Address of where the LED offset is stored in the NVRAM
boolean alarmTrig = false; // Whether the alarm has been triggered or not
uint32_t alarmTrigTime; // Milli seconds since the alarm was triggered
boolean countDown = false;
long countDownTime = 0;
long currentCountDown = 0;
long startCountDown;
int countDownMin;
int countDownSec;
int countDownFlash;
int demo_mode = 0;
volatile int j = 0;  // LED position in fast transition effects
long previousMillis = 0;
volatile uint8_t star = 0;
volatile float starBlinks;
volatile bool isBuzzerActive;  // Flag to avoid repetitive buzzer calls
BuzzerSequencer buzzer;  // Beep timing
volatile int8_t led_offset;  // Allows rotate clock by 90 segrees left/right 

State state = STATE_CLOCK; // State of the clock, see enum State
volatile uint8_t clockMode; // Variable of the display mode of the clock
volatile uint8_t alarmMode; // Variable of the alarm display mode

Bounce menuBouncer = Bounce(PIN_MENU,30); // Instantiate a Bounce object with a 50 millisecond debounce time for the menu button
boolean menuButton = false; 
boolean menuPressSeen = false;  // A press was observed since the last release
boolean menuIgnoreUntilRelease = false;  // The press that stopped the alarm must not act when released
volatile int16_t rotaryMove = 0;
volatile boolean countTime = false;
long menuTimePressed;
volatile long lastRotary;
int pendulumPos;

#define R_SHIFT	0
#define G_SHIFT	20
#define B_SHIFT	40
uint8_t color_intensity [] = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 5, 
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                              5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100};

// LED at position `i` of the clock face (0 = 12 o'clock), taking the mounting offset into account.
CRGB& led(int i) { return leds[(i + led_offset) % NUM_LEDS]; }

CRGB grey(uint8_t v) { return CRGB(v, v, v); }

void fillAll(const CRGB& color) { fill_solid(leds, NUM_LEDS, color); }

void clearLEDs() { fillAll(CRGB::Black); }

void printValue(const __FlashStringHelper* label, long value) {
  Serial.print(label);
  Serial.println(value);
}

// True in the "off" half of the flashing period. Used to flash the value being set.
bool blinkPhaseOff() { return millis() % BLINK_PERIOD_MS >= BLINK_PERIOD_MS / 2; }

// True (once) whenever more than `ms` passed since the last time it was true. Paces the animations.
bool intervalElapsed(long ms) {
  long now = millis();
  if (now - previousMillis <= ms) return false;
  previousMillis = now;
  return true;
}

// True when a new second has just started. Also measures the length of the previous second (it is not exactly 1000 ms).
bool startOfNewSecond(DateTime now) {
  if (now.second() == old.second()) return false;
  old = now;
  cyclesPerSec = millis() - newSecTime;
  cyclesPerSecFloat = (float) cyclesPerSec;
  newSecTime = millis();
  return true;
}

// How far we are into the current second: 0.0 .. 1.0
float secondProgress() { return (millis() - newSecTime) / cyclesPerSecFloat; }

// `count` blue LEDs starting at the top of the clock face.
void drawBlueRun(int count, uint8_t blue) {
  for (int i = 0; i < count; i++) led(i + 1).b = blue;
}

// Paint the first j LEDs (`filling`) or all the LEDs from j on, then advance j on schedule; next demo step at the end.
void colorWipe(const CRGB& color, bool filling) {
  if (filling) {
    for (int i = 0; i < j; i++) led(i + 1) = color;
  } else {
    for (int i = j; i < NUM_LEDS; i++) led(i + 1) = color;
  }
  if (intervalElapsed(TIME_INTERVAL)) j++;
  if (j == NUM_LEDS) {j = 0; demo_mode++;}
}

void drawMinuteAndSecond(DateTime now) {
  led(now.minute()).g = 255;
  led(now.second()).b = 255;
}

// Hour hand of the afternoon: red centre LED with dim neighbours.
// `solid`: the centre LED is pure red instead of just having its red channel raised.
void drawHourSpread(int pos, bool solid = false) {
  led(pos - 1).r = 50;
  if (solid) led(pos) = CRGB::Red;
  else led(pos).r = 255;
  led(pos + 1).r = 50;
}

// Hour hand for a 24h `hour`: a single LED before noon, a 3-LED spread after noon.
void drawHour24(uint8_t hour) {
  if (hour <= 11) led(hour * 5).r = 255;
  else drawHourSpread((hour - 12) * 5);
}

// Turn the hour hand off (the "flash" phase while setting the hour).
void hideHour(uint8_t hour) {
  int pos = (hour % 12) * 5;
  led(pos - 1).r = 0;
  led(pos).r = 0;
  led(pos + 1).r = 0;
}

void setup() {
  // Set up all pins
  // Attach after enabling the pull-up: the global Bounce object was constructed before this and latched a floating pin level.
  menuBouncer.attach(PIN_MENU, INPUT_PULLUP);  // Uses the internal 20k pull up resistor. Pre Arduino_v.1.0.1 need to be "digitalWrite(PIN_MENU,HIGH);pinMode(PIN_MENU,INPUT);"
    
  // Start LEDs
  LEDS.addLeds<WS2812B, PIN_LEDS, GRB>(leds, NUM_LEDS); // Structure of the LED data. I have changed to from rgb to grb, as using an alternative LED strip. Test & change these if you're getting different colours. 
  
  // Start RTC
  Wire.begin(); // Starts the Wire library allows I2C communication to the Real Time Clock
  RTC.begin(); // Starts communications to the RTC
  
  Serial.begin(9600); // Starts the serial communications

  // Uncomment to reset all the NVRAM addresses. You will have to comment again and reload, otherwise it will not save anything each time power is cycled
  // write a 0 to all 512 uint8_ts of the NVRAM
//  for (int i = 0; i < 512; i++)
//  {RTC.writenvram(i, 0);}

  // Load any saved setting since power off, such as mode & alarm time. Fix invalid values of a virgin device.
  Settings saved = sanitizeSettings({RTC.readnvram(CLOCK_MODE_ADDR), RTC.readnvram(ALARM_MIN_ADDR),
                                     RTC.readnvram(ALARM_HR_ADDR), RTC.readnvram(ALARM_SET_ADDR),
                                     RTC.readnvram(ALARM_MODE_ADDR), RTC.readnvram(LED_OFFSET_ADDR)},
                                    CLOCK_MODE_MAX);
  clockMode = saved.clockMode;
  alarmMin = saved.alarmMin;
  alarmHour = saved.alarmHour;
  alarmSet = saved.alarmSet;
  alarmMode = saved.alarmMode;
  led_offset = saved.ledOffset;
  // Write sanitized data back to NVRAM for further proper boot.
  RTC.writenvram(CLOCK_MODE_ADDR, clockMode); 
  RTC.writenvram(ALARM_MIN_ADDR, alarmMin); 
  RTC.writenvram(ALARM_HR_ADDR, alarmHour); 
  RTC.writenvram(ALARM_SET_ADDR, alarmSet); 
  RTC.writenvram(ALARM_MODE_ADDR, alarmMode);
  RTC.writenvram(LED_OFFSET_ADDR, led_offset);
  rotary1.write(0);  // Set non-interrupt mode to rotary
  state = STATE_CLOCK;

  // Print all the saved NVRAM data to Serial
  printValue(F("Mode is "), clockMode);
  printValue(F("Alarm Hour is "), alarmHour);
  printValue(F("Alarm Min is "), alarmMin);
  printValue(F("Alarm is set "), alarmSet);
  printValue(F("Alarm Mode is "), alarmMode);
  printValue(F("LED offset is "), led_offset);
  printDateTime();

  pinMode(PIN_BUZZER, OUTPUT);
  clearBuzzer();
}


void loop() {
  DateTime now = RTC.now(); // Fetches the time from RTC
  buzzerUpdate();

  // Check for any button presses and action accordingly
  menuButton = menuBouncer.update();  // Update the debouncer for the menu button and saves state to menuButton
  rotary1Pos = rotary1.read(); // Checks the rotary position
  if (rotary1Pos != 0) {
    if (millis() - lastRotary >= ROTARY_SET_TIME_MS) {
      rotaryMove = (rotary1Pos < 0)?-1:1;  // Limit moves to single step.
      rotary1.write(0);
      lastRotary = millis();
    } else {  // Reset position if no valid move detected.
      rotary1Pos = 0;
      rotary1.write(0);
    }
  }
  if (menuButton == true || rotaryMove != 0 || countTime == true) {buttonCheck(menuBouncer,now);}

  // clear LED array
  clearLEDs();  // Not memset(leds, ...): it is ambiguous with FastLED 3.10+
  
  // Check alarm and trigger if the time matches
  if (alarmSet == true && alarmDay != now.day()) { // The alarmDay statement ensures it is a newly set alarm or repeat from previous day, not within the minute of an alarm cancel.
    if (alarmTrig == false) {alarm(now);}
    else {alarmDisplay();}
  }
 // Check the Countdown Timer
  if (countDown == true) {
    currentCountDown = countdownRemaining(countDownTime, startCountDown, now.unixtime());
    if ( currentCountDown <= 0) state = STATE_COUNTDOWN;
  } 
  // Set the time LED's
  if (state == STATE_SET_CLOCK_HR || state == STATE_SET_CLOCK_MIN || state == STATE_SET_CLOCK_SEC || state == STATE_SET_CLOCK_UP) {setClockDisplay(now);}
  else if (state == STATE_ALARM || state == STATE_SET_ALARM_HR || state == STATE_SET_ALARM_MIN) {setAlarmDisplay();}
  else if (state == STATE_COUNTDOWN) {countDownDisplay(now);}
  else if (state == STATE_DEMO) {runDemo(now);}
  else {timeDisplay(now);}

  // Update LEDs
  LEDS.show();
}

void printDateTime() {
  DateTime now = RTC.now();
  // Sanity check for stored time
  if (now.hour() > 23 || now.minute() > 59 || now.second() > 59 || now.month() > 12 || now.day() > 31) {
    RTC.adjust(DateTime(2018, 1, 1, 1, 1, 1));
    now = RTC.now();
    Serial.println(F("Fixed date and time."));
  }      
  printValue(F("Hour time is... "), now.hour());
  printValue(F("Min time is... "), now.minute());
  printValue(F("Sec time is... "), now.second());
  printValue(F("Year is... "), now.year());
  printValue(F("Month is... "), now.month());
  printValue(F("Day is... "), now.day());
}

// Copy the live globals into the model the menu state machine works on ...
MenuModel loadMenuModel() {
  MenuModel m;
  m.state = state;
  m.clockMode = clockMode;
  m.alarmMode = alarmMode;
  m.alarmSet = alarmSet;
  m.alarmHour = alarmHour;
  m.alarmMin = alarmMin;
  m.alarmDay = alarmDay;
  m.ledOffset = led_offset;
  m.alarmTrig = alarmTrig;
  m.countDown = countDown;
  m.countDownTime = countDownTime;
  m.currentCountDown = currentCountDown;
  m.startCountDown = startCountDown;
  m.rotaryMove = rotaryMove;
  m.countTime = countTime;
  m.menuPressSeen = menuPressSeen;
  m.menuTimePressed = menuTimePressed;
  m.ignoreUntilRelease = menuIgnoreUntilRelease;
  return m;
}

// ... and back.
void storeMenuModel(const MenuModel& m) {
  state = m.state;
  clockMode = m.clockMode;
  alarmMode = m.alarmMode;
  alarmSet = m.alarmSet;
  alarmHour = m.alarmHour;
  alarmMin = m.alarmMin;
  alarmDay = m.alarmDay;
  led_offset = m.ledOffset;
  alarmTrig = m.alarmTrig;
  countDown = m.countDown;
  countDownTime = m.countDownTime;
  currentCountDown = m.currentCountDown;
  startCountDown = m.startCountDown;
  rotaryMove = m.rotaryMove;
  countTime = m.countTime;
  menuPressSeen = m.menuPressSeen;
  menuTimePressed = m.menuTimePressed;
  menuIgnoreUntilRelease = m.ignoreUntilRelease;
}

// Carry out what the menu state machine asked for.
void applyMenuEffects(const MenuEffects& fx) {
  if (fx.blinkDisplay) {  // blink display while button is pressed to indicate "entered adjust mode"
    clearLEDs();
    LEDS.show();
    delay(100);
  }
  if (fx.setTime) {
    RTC.adjust(DateTime(fx.newTime.year, fx.newTime.month, fx.newTime.day, fx.newTime.hour, fx.newTime.minute, fx.newTime.second));
  }
  if (fx.clearBuzzer) clearBuzzer();
  if (fx.stopBuzzer) buzzerStop();
  if (fx.resetSecTimer) newSecTime = millis();
  if (fx.resetJ) j = 0;
  if (fx.startDemo) demo_mode = 1;
  if (fx.saveClockMode) RTC.writenvram(CLOCK_MODE_ADDR, clockMode);
  if (fx.saveAlarmSet) RTC.writenvram(ALARM_SET_ADDR, alarmSet);
  if (fx.saveAlarmMode) RTC.writenvram(ALARM_MODE_ADDR, alarmMode);
  if (fx.saveAlarmHour) RTC.writenvram(ALARM_HR_ADDR, alarmHour);
  if (fx.saveAlarmMin) RTC.writenvram(ALARM_MIN_ADDR, alarmMin);
  if (fx.saveLedOffset) RTC.writenvram(LED_OFFSET_ADDR, led_offset);
  if (fx.printDateTime) printDateTime();
  if (fx.waitAfterAlarmCancel) delay(300);  // let time for the button to be released
}

void buttonCheck(Bounce& button, DateTime now) {
  MenuInput in = {};
  in.pressed = !button.read();
  in.heldMs = in.pressed ? button.currentDuration() : 0;
  in.released = button.rose();
  in.clockModeCount = CLOCK_MODE_MAX;
  in.now.year = now.year();
  in.now.month = now.month();
  in.now.day = now.day();
  in.now.hour = now.hour();
  in.now.minute = now.minute();
  in.now.second = now.second();
  in.now.unixtime = now.unixtime();

  State stateBefore = state;
  MenuModel model = loadMenuModel();
  MenuEffects fx = menuStep(model, in);
  storeMenuModel(model);
  applyMenuEffects(fx);

  if (stateBefore == STATE_ALARM && !fx.waitAfterAlarmCancel) {
    printValue(F("alarmSet is "), alarmSet);
    printValue(F("alarmMode is "), alarmMode);
  }
  if (!fx.waitAfterAlarmCancel) {
    printValue(F("Mode is "), clockMode);
    printValue(F("State is "), (int)state);
  }
}

void setAlarmDisplay() {
  // Background: 5-minute ticks. Red = alarm is off, green = alarm is on.
  CRGB tick = alarmSet ? CRGB(0, 20, 0) : CRGB(20, 0, 0);
  for (int i = 0; i < NUM_LEDS; i += 5) led(i) = tick;
  drawHour24(alarmHour);
  led(alarmMin).g = 100;
  // Turn off the value being set periodically to show flashing.
  bool off = blinkPhaseOff();
  if (state == STATE_SET_ALARM_HR && off) hideHour(alarmHour);
  if (state == STATE_SET_ALARM_MIN && off) led(alarmMin).g = 0;
  led(alarmMode).b = 255;
}

void setClockDisplay(DateTime now) {
  fill_ticks(10);
  drawHour24(now.hour());
  bool off = blinkPhaseOff();
  if (state == STATE_SET_CLOCK_HR && off) hideHour(now.hour());
  if (state == STATE_SET_CLOCK_MIN && off) led(now.minute()).g = 0;
  else led(now.minute()).g = 255;
  if (state == STATE_SET_CLOCK_SEC && off) led(now.second()).b = 0;
  else led(now.second()).b = 255;
}

// Tone is generated by Timer1 in CTC mode, toggling OC1A (D9) in hardware.
// No interrupts involved, so FastLED.show() (interrupts off) can't distort the tone.
void toneOn() {
  OCR1A = F_CPU / (2UL * 8 * BEEP_TONE) - 1;
  TCNT1 = 0;
  TCCR1A = _BV(COM1A0);              // Toggle OC1A on compare match
  TCCR1B = _BV(WGM12) | _BV(CS11);   // CTC, prescaler 8
}

void toneOff() {
  TCCR1A = 0;
  TCCR1B = 0;
  PORTB &= ~_BV(PORTB1);             // Keep buzzer pin low
}

// Carry out what the beep sequencer asked for.
void applyBuzzerEvent(const BuzzerEvent& e) {
  if (e.setTone) {
    if (e.tone) toneOn();
    else toneOff();
  }
  if (e.finished) clearBuzzer();  // The timer pattern is over.
}

void buzzerUpdate() {
  applyBuzzerEvent(sequencerTick(buzzer, millis()));
}

void buzzerStop() {
  sequencerStop(buzzer);
  toneOff();
}

// Signal buzzer is ready for next action.
void clearBuzzer() {
  buzzerStop();
  isBuzzerActive = false;
  // Stop triggers, return to clock mode.
  state = STATE_CLOCK;
  countDown = false;
}

// Enable buzzer sequence only once on reqest until buzzer operation is cleared. Avoid retriggering buzzer.
void runBuzzer(int mode) {
  if (isBuzzerActive) return;
  // Timer: 5 double beeps. Alarm: 3 beeps endlessly (until rotary rotate).
  applyBuzzerEvent(sequencerStart(buzzer, (TIMER == mode) ? TIMER_BEEP : ALARM_BEEP, millis()));
  isBuzzerActive = true;
}

void alarm(DateTime now) {
  // Don't alarm if in set mode.
  if (state == STATE_SET_ALARM_HR || state == STATE_SET_ALARM_MIN) return;

  // Alarm if not in set mode and alarm time is now.
  if ((alarmMin == now.minute()%60) && (alarmHour == now.hour()%24)) {
    alarmTrig = true;
    alarmTrigTime = millis();
  }
}

void alarmDisplay() {
  runBuzzer(ALARM);
  if (alarmMode >= 1 && alarmMode <= ALARM_MODE_MAX) ALARM_EFFECTS[alarmMode - 1]();
}

// Dim white on all LEDs: avoid overcurrent.
void alarmSolid() {
  fillAll(grey(20));
}

// White grows from both ends of the ring, then fills everything.
void alarmRamp() {
  int16_t pos = alarmRampPosition(millis() - alarmTrigTime);
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
void alarmFade() {
  fillAll(grey(alarmFadeBrightness(millis() - alarmTrigTime)));
}

void countDownDisplay(DateTime now) {
  if (countDown == true) {
    // Counting down
    currentCountDown = countdownRemaining(countDownTime, startCountDown, now.unixtime());
    if (currentCountDown > 0) {
      // Time is not gone yet, decrement lights
      countDownMin = currentCountDown / 60;
      countDownSec = currentCountDown%60 * 2; // Range 0-120 to create brightness less than 240
      drawBlueRun(countDownMin, 240); // A blue LED for each complete minute that is remaining
      led(countDownMin+1).b = countDownSec; // Display the remaining secconds of the current minute as its brightness
    } else {
      // Time is gone, flash and beep.
      countDownFlash = now.unixtime()%2;
      runBuzzer(TIMER);
      if (countDownFlash == 0) clearLEDs();
      else fillAll(CRGB::Blue);  // Set the background as all blue
    }
  } else {
    // Setting mode
    currentCountDown = countDownTime;
    if (countDownTime == 0) {
      clearLEDs();
      switch (demo_mode) {
        case 0:
          drawBlueRun(j, 20);
          if (intervalElapsed(TIME_INTERVAL)) j++;
          if (j == NUM_LEDS) {demo_mode = 1;}
          break;
        case 1:
          drawBlueRun(j, 20);
          if (intervalElapsed(TIME_INTERVAL)) j--;
          if (j < 0) {demo_mode = 0;}
          break;
        default:
          demo_mode = 0;
      }
    } else if (countDownTime > 0 && blinkPhaseOff()) {
      countDownMin = currentCountDown / 60;
      drawBlueRun(countDownMin, 255); // A blue LED for each complete minute that is remaining
    }
  }
}

void runDemo(DateTime now) {
  currentDemoTime = now.unixtime();
  clearLEDs();
  switch (demo_mode) {
    case 0:
      timeDisplay(now);
      if (currentDemoTime - previousDemoTime > DEMO_TIME_S) {previousDemoTime = currentDemoTime;}
      break;
    case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:
      colorWipe(DEMO_COLORS[demo_mode], demo_mode % 2 == 1);  // Odd steps paint the colour in, even steps wipe it out
      break;
    case 9:
      rainbow();
      if (intervalElapsed(TIME_INTERVAL * 5000)) {demo_mode = 1; j = 0;}
      break;
  }
}

void timeDisplay(DateTime now) {
  if (clockMode < CLOCK_MODE_MAX) FACES[clockMode](now);
  else clockMode = RTC.readnvram(CLOCK_MODE_ADDR);  // Out of range: restore the saved mode.
}

////////////////////////////////////////////////////////////////////////////////////////////
//   CLOCK DISPLAY MODES
// Add any new display mode functions here. Then add to the "void timeDisplay(DateTime now)" function.
// Add each of the new display mode functions as a new "case", leaving default last.
////////////////////////////////////////////////////////////////////////////////////////////

// Just 3 LEDs on.
void minimalClock(DateTime now) {
  unsigned char hourPos = (now.hour()%12)*5;
  led(hourPos).r = 255;
  drawMinuteAndSecond(now);
}

// # Red LEDs for hours, 1 LED per min and sec.
void basicClock(DateTime now) {
  drawHourSpread(hourHandPosition(now.hour(), now.minute()), true);
  // Mix colors if the same LED is chosen
  drawMinuteAndSecond(now);
}

// Second hand flowing from sec-to-sec + Basic clock
void smoothSecond(DateTime now) {
  basicClock(now);
  startOfNewSecond(now);
  // set hour, min & sec LEDs
  fracOfSec = secondProgress();  // This divides by 733, but should be 1000 and not sure why???
  if (subSeconds < cyclesPerSec) { brightness = 50.0*(1.0+sin((PI*fracOfSec)-HALF_PI)); }
  led(now.second()).b = brightness;
  led(now.second()-1).b = 100 - brightness;
}

// Constant lit 5-minute ticks + Basic clock
void outlineClock(DateTime now) {
  fill_ticks(100);
  basicClock(now);
}

void starryNightClock(DateTime now) {
  basicClock(now);
  if (startOfNewSecond(now)) {
    star = random8(NUM_LEDS);           // Choose star
    starBlinks = (float)(random8(1, 4) * 8);   // Choose times of sparkles
  }
  float m = (float) (millis() % 2000) / 3000.0;
  brightness = (2.0-m)*15.0*(1.0+sin(m*starBlinks-0.7));
  brightness = min(brightness, 100);  // cut numbers > 100
  brightness = (brightness < 15)?0:brightness;  // cut numbers < 15
  leds[star] = grey(brightness);
}

// Running white light over clock round. Full round in 1 second.
void minimalMilliSec(DateTime now) {
  startOfNewSecond(now);
  // set hour, min & sec LEDs
  subSeconds = (((millis() - newSecTime)*60)/cyclesPerSec)%60;  // This divides by 733, but should be 1000 and not sure why???
  // Millisec lights are set first, so hour/min/sec lights override and don't flicker as millisec passes
  led(subSeconds) = grey(50);
  // The colours are set last, so if on same LED mixed colours are created
  drawHourSpread(hourHandPosition(now.hour(), now.minute()));
  drawMinuteAndSecond(now);
}

// Pendulum will be at the bottom and left for one second and right for one second
void simplePendulum(DateTime now) {
  basicClock(now);
  if (startOfNewSecond(now)) swingBack = -swingBack;
  fracOfSec = secondProgress();  // This divides by 733, but should be 1000 and not sure why???
  if (subSeconds < cyclesPerSec) {
    pendulumPos = pendulumLed(fracOfSec, swingBack, led_offset);
  }
  // Pendulum lights are set first, so hour/min/sec lights override and don't flicker as millisec passes
  leds[pendulumPos] = CRGB::WhiteSmoke;
}

void breathingClock(DateTime now) {
  brightness = 30.0*(1.0+sin((PI*millis()/2000.0)-HALF_PI)) + 2;
  fill_ticks(brightness);
  basicClock(now);
}

void fill_ticks(uint8_t bright) {
  for (int i = 0; i < NUM_LEDS; i += 5) led(i) = grey(bright);  // 5-minute ticks
}

void rainbow() {
  // Set/progress initial position for the rainbow.
  star++;
  star = star % NUM_LEDS;

  // Fill the rainbow
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].r = color_intensity[(star + i + R_SHIFT) % NUM_LEDS];
    leds[i].g = color_intensity[(star + i + G_SHIFT) % NUM_LEDS];
    leds[i].b = color_intensity[(star + i + B_SHIFT) % NUM_LEDS];
  }
  delay(10);
}

