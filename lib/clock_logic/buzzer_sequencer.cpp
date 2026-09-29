#include "buzzer_sequencer.h"

BuzzerEvent sequencerStart(BuzzerSequencer& s, const BuzzerPattern& p, uint32_t nowMs) {
  s.pattern = p;
  s.beepIdx = 0;
  s.cycleIdx = 0;
  s.running = true;
  s.toneOn = true;
  s.nextMs = nowMs + p.onMs;
  return BuzzerEvent{true, true, false};
}

BuzzerEvent sequencerTick(BuzzerSequencer& s, uint32_t nowMs) {
  BuzzerEvent e = {};
  if (!s.running) return e;
  if ((int32_t)(nowMs - s.nextMs) < 0) return e;  // Not due yet. Safe across the millis() rollover.

  if (s.toneOn) {  // End of a beep
    s.toneOn = false;
    e.setTone = true;
    e.tone = false;
    if (++s.beepIdx < s.pattern.beeps) {
      s.nextMs = nowMs + s.pattern.offMs;
    } else {
      s.beepIdx = 0;
      if (s.pattern.cycles && ++s.cycleIdx >= s.pattern.cycles) {
        s.running = false;
        e.finished = true;
        return e;
      }
      s.nextMs = nowMs + s.pattern.pauseMs;
    }
  } else {  // End of a gap
    s.toneOn = true;
    e.setTone = true;
    e.tone = true;
    s.nextMs = nowMs + s.pattern.onMs;
  }
  return e;
}

void sequencerStop(BuzzerSequencer& s) {
  s.running = false;
  s.toneOn = false;
}
