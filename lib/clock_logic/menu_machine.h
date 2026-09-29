// Menu state machine: button + rotary input in, new clock state and hardware requests out.
// No Arduino, no hardware: compiles for the Nano and for the native (PC) unit tests.
#pragma once
#include "clock_logic.h"

// Wall-clock time as needed by the menu (copied from the RTC).
struct NowInfo {
  uint16_t year;
  uint8_t month, day, hour, minute, second;
  uint32_t unixtime;
};

// Everything the menu can change. The sketch keeps the live values in globals and copies them in and out.
struct MenuModel {
  State state = STATE_CLOCK;
  uint8_t clockMode = 0;
  uint8_t alarmMode = 0;      // 0 = alarm off
  uint8_t alarmSet = 0;
  uint8_t alarmHour = 0;
  uint8_t alarmMin = 0;
  uint8_t alarmDay = 0;       // Day the alarm was last cancelled on (0 = none)
  uint8_t ledOffset = LED_OFFSET_L;
  bool alarmTrig = false;     // The alarm is ringing
  bool countDown = false;     // The timer is running
  long countDownTime = 0;     // Timer length in seconds
  long currentCountDown = 0;  // Seconds left on the timer
  long startCountDown = 0;    // Unix time the timer was started at
  int16_t rotaryMove = 0;     // Pending rotary step: -1, 0, +1
  bool countTime = false;     // The button is held
  bool menuPressSeen = false; // A press was observed since the last release
  long menuTimePressed = 0;   // Duration of the last press, ms
  bool ignoreUntilRelease = false;  // The press that stopped the alarm is still going: swallow it up to its release
};

struct MenuInput {
  bool pressed;               // The button is held down right now
  long heldMs;                // How long it has been held (only meaningful while pressed)
  bool released;              // The button was released since the previous call
  uint8_t clockModeCount;     // Number of clock faces
  NowInfo now;
};

// What the sketch has to do with the hardware after a step.
struct MenuEffects {
  bool blinkDisplay;          // Blank the LEDs briefly: "entered adjust mode"
  bool clearBuzzer;           // Stop the buzzer and forget its state
  bool stopBuzzer;            // Stop the buzzer only
  bool waitAfterAlarmCancel;  // Give the user time to release the button
  bool resetSecTimer;         // Restart the sub-second timer
  bool resetSweep;            // Restart the timer's idle sweep animation
  bool startDemo;             // Begin the demo from its first colour
  bool printDateTime;
  bool setTime;               // Write newTime to the RTC
  NowInfo newTime;
  bool saveClockMode, saveAlarmSet, saveAlarmMode, saveAlarmHour, saveAlarmMin, saveLedOffset;
};

MenuEffects menuStep(MenuModel& m, const MenuInput& in);

// What the alarm is doing right now.
enum AlarmPhase : uint8_t {
  ALARM_IDLE,     // Off, not the time yet, or already cancelled today
  ALARM_DUE,      // It is time: start ringing
  ALARM_RINGING   // Ringing until the user stops it
};

AlarmPhase alarmPhase(const MenuModel& m, const NowInfo& now);
