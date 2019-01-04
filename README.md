# infinity-clock

This repo follows and inspired by http://barkengmad.com/rise-and-shine-led-clock/

Original Morgan Barke's video:
![See the magic](https://youtu.be/YErWfe0aTiQ "Yoohoo!")

This project fixes few issues in original clock and improves usability, user experience, and build ease.

# Usage
See the PDF chart.

# Changes
- Postpone write to EEPROM by 5 seconds: let user to complete the settings and then write. This saves EEPROM wear out.
- Move rotary button from A3 to D4: this helps ease of assembly. This change is *not shown* in the breadboard PNG.
- Remove excessive Serial prints.
- Refactor menu button work. Less code, easy readable.
- Keep clock mode and restore it after power failure.

# Build/compile
Original clock comes with libraries in archive. The libraries are outdated. Install in your Arduino/Platformio environment following libraries _before_ compiling:
- FastLED
- Bounce2
- RTCLib
- Encoder of Paul Stoffregen

Likely, following libs are preinstalled by default:
- EEPROM
- Wire

# TODO
- Move alarm to RTC. This allows daily alarm.
