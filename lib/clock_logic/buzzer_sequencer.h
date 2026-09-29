// Non-blocking beep sequencer: decides when the tone must switch on and off. Time is passed in, and the result is
// returned as an event, so it has no Arduino or hardware dependency and is unit-tested on the PC.
#pragma once
#include <stdint.h>

// `beeps` beeps of `onMs` separated by `offMs`, then `pauseMs` of silence, repeated `cycles` times (0 = endless).
struct BuzzerPattern {
  uint16_t onMs;
  uint16_t offMs;
  uint8_t beeps;
  uint16_t pauseMs;
  uint8_t cycles;
};

const BuzzerPattern TIMER_BEEP = {300, 500, 2, 1000, 5};  // End of timer: 5 double beeps
const BuzzerPattern ALARM_BEEP = {300, 200, 3, 1000, 0};  // Alarm: 3 beeps, endlessly (until stopped)

struct BuzzerSequencer {
  BuzzerPattern pattern = {};
  bool running = false;
  bool toneOn = false;
  uint8_t beepIdx = 0;
  uint8_t cycleIdx = 0;
  uint32_t nextMs = 0;  // When the next switch is due
};

// What the caller has to do after start/tick.
struct BuzzerEvent {
  bool setTone;   // Switch the tone to `tone`
  bool tone;
  bool finished;  // The last cycle is over (never for an endless pattern)
};

// Begin the pattern with a beep right away.
BuzzerEvent sequencerStart(BuzzerSequencer& s, const BuzzerPattern& p, uint32_t nowMs);

// Call as often as possible with the current time.
BuzzerEvent sequencerTick(BuzzerSequencer& s, uint32_t nowMs);

// Forget the pattern. The caller switches the tone off.
void sequencerStop(BuzzerSequencer& s);
