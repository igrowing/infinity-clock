// Debug output on the serial port.
#pragma once
#include <Arduino.h>

// "label value" on its own line. The label lives in flash: printValue(F("Mode is "), mode).
inline void printValue(const __FlashStringHelper* label, long value) {
  Serial.print(label);
  Serial.println(value);
}
