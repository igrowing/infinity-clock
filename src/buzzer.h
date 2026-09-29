// The buzzer: the tone comes from Timer1 in hardware, the beep timing from the tested BuzzerSequencer.
#pragma once
#include <Arduino.h>

enum BuzzerMode : uint8_t {
  BUZZER_TIMER,   // End of timer: 5 double beeps
  BUZZER_ALARM    // Alarm: 3 beeps, endlessly, until stopped
};

void buzzerBegin();

// Call as often as possible. True when the timer pattern has just finished.
bool buzzerUpdate();

// Start the beeps. Does nothing while a sequence is already active, until buzzerReset().
void runBuzzer(BuzzerMode mode);

// Silence the buzzer.
void buzzerStop();

// Silence the buzzer and allow runBuzzer() to start a new sequence.
void buzzerReset();
