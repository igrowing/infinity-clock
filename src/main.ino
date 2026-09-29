/***********************************************************
 * Inspired by http://barkengmad.com/rise-and-shine-led-clock/
 * and based Morgan Barke's code.
 *
 * Modded by iGrowing 2019.
 * https://github.com/igrowing/infinity-clock
 *
 * Layout of the code:
 *   main.ino    setup, loop and the glue between the buttons, the menu and the hardware
 *   leds        the LED ring and the drawing primitives
 *   faces       the different ways of showing the time
 *   screens     setting screens, timer, demo and the alarm effects
 *   buzzer      the sound
 *   settings    what is kept in the NVRAM of the RTC
 *   lib/clock_logic   pure logic, unit-tested on the PC: menu state machine, beep timing, alarm decision, ...
 ***********************************************************/

//Add the following libraries to the respective folder for you operating system. See http://arduino.cc/en/Guide/Environment
#include <FastLED.h> // FastSPI Library version 3.1.X from https://github.com/FastLED/FastLED. Version 3.0.X has a bug: all LEDs are green by default.
#include <Wire.h> //This is to communicate via I2C. On arduino Uno & Nano use pins A4 for SDA (yellow/orange) and A5 for SCL (green). For other boards ee http://arduino.cc/en/Reference/Wire
#include <RTClib.h>           // Include the RTClib library to enable communication with the real time clock.
#include <Bounce2.h>          // Include the Bounce library for de-bouncing issues with push buttons.
#include <Encoder.h>          // Include the Encoder library to read the out puts of the rotary encoders
#include <menu_machine.h>     // Menu state machine, unit-tested on PC
#include "config.h"
#include "log.h"
#include "rtc.h"
#include "leds.h"
#include "faces.h"
#include "screens.h"
#include "buzzer.h"
#include "settings.h"

RTC_DS1307 RTC;     // Establishes the chipset of the Real Time Clock
MenuModel menu;     // Everything the user can change: state, modes, alarm, timer
Encoder rotary1(PIN2, PIN3); // Setting up the Rotary Encoder
Bounce menuBouncer = Bounce(PIN_MENU,30); // Instantiate a Bounce object with a 30 millisecond debounce time for the menu button
unsigned long lastRotary = 0;

// Silence the buzzer, forget any ringing state and go back to the clock.
void clearBuzzer() {
  buzzerReset();
  menu.state = STATE_CLOCK;
  menu.countDown = false;
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

void setup() {
  // Attach after enabling the pull-up: the global Bounce object was constructed before this and latched a floating pin level.
  menuBouncer.attach(PIN_MENU, INPUT_PULLUP);  // Uses the internal 20k pull up resistor. Pre Arduino_v.1.0.1 need to be "digitalWrite(PIN_MENU,HIGH);pinMode(PIN_MENU,INPUT);"

  // Start LEDs
  LEDS.addLeds<WS2812B, PIN_LEDS, GRB>(leds, NUM_LEDS); // Structure of the LED data. I have changed to from rgb to grb, as using an alternative LED strip. Test & change these if you're getting different colours.

  // Start RTC
  Wire.begin(); // Starts the Wire library allows I2C communication to the Real Time Clock
  RTC.begin(); // Starts communications to the RTC

  Serial.begin(9600); // Starts the serial communications

  // Load any saved setting since power off, such as mode & alarm time
  loadSettings(menu);
  setLedOffset(menu.ledOffset);
  rotary1.write(0);  // Set non-interrupt mode to rotary
  menu.state = STATE_CLOCK;

  printSettings(menu);
  printDateTime();

  buzzerBegin();
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
  if (fx.resetSecTimer) restartSecondTimer();
  if (fx.resetSweep) resetSweep();
  if (fx.startDemo) startDemo();
  if (fx.saveLedOffset) setLedOffset(menu.ledOffset);
  saveSettings(menu, fx);
  if (fx.printDateTime) printDateTime();
  if (fx.waitAfterAlarmCancel) delay(300);  // let time for the button to be released
}

// Feed the state of the button into the menu state machine and act on the result.
void buttonCheck(Bounce& button, DateTime now) {
  MenuInput in = {};
  in.pressed = !button.read();
  in.heldMs = in.pressed ? button.currentDuration() : 0;
  in.released = button.rose();
  in.clockModeCount = clockFaceCount();
  in.now = toNowInfo(now);

  State stateBefore = menu.state;
  MenuEffects fx = menuStep(menu, in);
  applyMenuEffects(fx);

  if (stateBefore == STATE_ALARM && !fx.waitAfterAlarmCancel) {
    printValue(F("alarmSet is "), menu.alarmSet);
    printValue(F("alarmMode is "), menu.alarmMode);
  }
  if (!fx.waitAfterAlarmCancel) {
    printValue(F("Mode is "), menu.clockMode);
    printValue(F("State is "), (int)menu.state);
  }
}

void loop() {
  DateTime now = RTC.now(); // Fetches the time from RTC
  if (buzzerUpdate()) clearBuzzer();  // The timer beeps are over

  // Check for any button presses and action accordingly
  bool buttonChanged = menuBouncer.update();  // Update the debouncer for the menu button
  int rotaryPos = rotary1.read(); // Checks the rotary position
  if (rotaryPos != 0) {
    if (millis() - lastRotary >= ROTARY_SET_TIME_MS) {
      menu.rotaryMove = (rotaryPos < 0)?-1:1;  // Limit moves to single step.
      lastRotary = millis();
    }
    rotary1.write(0);  // Reset position, whether the move was valid or not.
  }
  if (buttonChanged || menu.rotaryMove != 0 || menu.countTime) {buttonCheck(menuBouncer, now);}

  clearLEDs();  // Not memset(leds, ...): it is ambiguous with FastLED 3.10+

  // Check alarm and trigger if the time matches
  switch (alarmPhase(menu, toNowInfo(now))) {
    case ALARM_DUE:
      menu.alarmTrig = true;
      alarmStarted();
      break;
    case ALARM_RINGING:
      alarmDisplay(menu.alarmMode);
      break;
    case ALARM_IDLE:
      break;
  }

  // Check the Countdown Timer
  if (menu.countDown) {
    menu.currentCountDown = countdownRemaining(menu.countDownTime, menu.startCountDown, now.unixtime());
    if (menu.currentCountDown <= 0) menu.state = STATE_COUNTDOWN;
  }

  // Set the LEDs of the current screen
  if (!renderScreen(menu, now)) menu.clockMode = RTC.readnvram(CLOCK_MODE_ADDR);  // Invalid face: restore the saved one.

  // Update LEDs
  LEDS.show();
}
