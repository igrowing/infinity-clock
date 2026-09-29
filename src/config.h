// Pins, timing constants and NVRAM layout of the clock.
#pragma once
#include <Arduino.h>
#include <clock_logic.h>   // NUM_LEDS, LED_OFFSET_L/R, HOLD_TIME_MS, ...

#define BEEP_TONE   700    // Hz
#define PIN_BUZZER  9      // OC1A (Timer1 hardware output). Must be D9 for the hardware tone.
#define PIN_LEDS    A0
#define PIN_MENU    PIN4
#define ROTARY_SET_TIME_MS 300
#define TIME_INTERVAL 5        // ms between steps of the LED animations
#define BLINK_PERIOD_MS 300    // Flashing while a value is being set: half of it off, half on

// Where the settings live in the NVRAM of the RTC
#define CLOCK_MODE_ADDR  0
#define ALARM_MIN_ADDR   1
#define ALARM_HR_ADDR    2
#define ALARM_SET_ADDR   3
#define ALARM_MODE_ADDR  4
#define LED_OFFSET_ADDR  5
