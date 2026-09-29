// Paces an animation. Every animation owns its own Ticker, so they can't disturb each other.
#pragma once
#include <Arduino.h>

struct Ticker {
  unsigned long last = 0;

  // True (once) whenever more than `ms` passed since the last time it was true.
  bool elapsed(unsigned long ms) {
    unsigned long now = millis();
    if (now - last <= ms) return false;
    last = now;
    return true;
  }

  // Start counting from now.
  void restart() { last = millis(); }
};
