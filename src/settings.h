// The settings that survive a power cycle. They live in the NVRAM of the RTC.
#pragma once
#include <menu_machine.h>

// Read the saved settings into the model. Invalid values (a virgin device) are replaced by defaults, and the
// corrected values are written back so that the next boot is fine.
void loadSettings(MenuModel& m);

// Save what the last menu step changed.
void saveSettings(const MenuModel& m, const MenuEffects& fx);

// Print the settings to the serial port.
void printSettings(const MenuModel& m);
