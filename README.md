# infinity-clock

This repo follows and inspired by http://barkengmad.com/rise-and-shine-led-clock/

Original Morgan Barke's video:
![See the magic](https://youtu.be/YErWfe0aTiQ "Yoohoo!")

I like the original inifinity clock after reviewing dosen (or even more) similar devices. I appreciate Morgan's creativity and generosity making his project public. Sharing is caring, and I'm giving back :)

This project fixes few issues in original clock and improves usability, user experience, and build ease.

# Usage
See the PDF chart.

The clock consumes about 600mA @ 5V in worst case (white demo). Therefore, usual 1A USB charger is enough to power it up.

# Changes vs. original clock
- Use RTC NVRAM instead of EEPROM: this saves EEPROM wear out.
- Move rotary button from A3 to D4: this helps ease of assembly. This change is *not shown* in the breadboard PNG.
- Remove excessive Serial prints.
- Refactor menu button work. Less code, easy readable.
- Refactor and make stable the rotary encoder.
- Keep clock mode and restore it after power failure.
- Breathing clock is more distinctive.
- Numerous bugs fixed.

# Build/compile
Original clock comes with libraries in archive. The libraries are outdated. Install in your Arduino/Platformio environment following libraries _before_ compiling:
- FastLED v3.2.1 and up
- Bounce2
- RTCLib
- Encoder of Paul Stoffregen

Likely, following libs are preinstalled by default:
- Wire

# TODO
- Move alarm to RTC. This allows daily alarm.
