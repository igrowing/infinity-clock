// Everything the LEDs can show besides a plain clock face: the setting screens, the timer, the demo and
// the effects that play while the alarm rings. Each animation keeps its own state in screens.cpp.
#pragma once
#include <RTClib.h>
#include <menu_machine.h>

// Draw the screen that belongs to the current state of the menu. False if the clock face number is invalid.
// (The timer screen also keeps the remaining time of the model up to date.)
bool renderScreen(MenuModel& m, DateTime now);

// The alarm just started ringing.
void alarmStarted();

// Sound and light of a ringing alarm. `alarmMode` 1..ALARM_MODE_MAX selects the effect.
void alarmDisplay(uint8_t alarmMode);

// Restart the idle animation of the timer screen.
void resetSweep();

// Start the demo from its first colour.
void startDemo();
