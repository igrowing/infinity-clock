/***********************************************************
 * Inspired by http://barkengmad.com/rise-and-shine-led-clock/
 * and based Morgan Barke's code.
 * 
 * Modded by iGrowing 2019.
 * https://github.com/igrowing/infinity-clock
 ***********************************************************/

//Add the following libraries to the respective folder for you operating system. See http://arduino.cc/en/Guide/Environment
#include <FastLED.h> // FastSPI Library version 3.1.X from https://github.com/FastLED/FastLED. Version 3.0.X has a bug: all LEDs are green by default.
#include <Wire.h> //This is to communicate via I2C. On arduino Uno & Nano use pins A4 for SDA (yellow/orange) and A5 for SCL (green). For other boards ee http://arduino.cc/en/Reference/Wire
#include <RTClib.h>           // Include the RTClib library to enable communication with the real time clock.
#include <Bounce2.h>          // Include the Bounce library for de-bouncing issues with push buttons.
#include <Encoder.h>          // Include the Encoder library to read the out puts of the rotary encoders
#include <EasyBuzzer.h>

RTC_DS1307 RTC;     // Establishes the chipset of the Real Time Clock

#define TIMER       1      // Modes of buzzer
#define ALARM       2
#define BEEP_TONE   2100   // Hz
#define PIN_BUZZER  A1
#define PIN_LEDS    A0
#define PIN_MENU    PIN4
#define NUM_LEDS    60    // Number of LEDs in strip
#define LED_OFFSET_L 0    // Adjust by LED position/shift in the circle
#define LED_OFFSET_R 45  // Adjust by LED position/shift in the circle
#define HOLD_TIME_MS 1500
#define DEMO_TIME_S 12 // seconds
#define ROTARY_SET_TIME_MS 300
#define TIME_INTERVAL 5
#define FADE_TIME_MS 60000

struct CRGB leds[NUM_LEDS];  // Setting up the LED strip
Encoder rotary1(PIN2, PIN3); // Setting up the Rotary Encoder

DateTime old; // Variable to compare new and old time, to see if it has moved on.
int rotary1Pos = 0;
int subSeconds; // 60th's of a second
int brightness = 0;
long newSecTime; // Variable to record when a new second starts, allowing to create milli seconds
long flashTime;
long breathCycleTime;
int cyclesPerSec;
float cyclesPerSecFloat;
float fracOfSec;
float breathFracOfSec;
boolean demo;
long previousDemoTime;
long currentDemoTime;
float swingBack = 1.57;

volatile uint8_t alarmMin; // The minute of the alarm  
volatile uint8_t alarmHour; // The hour of the alarm 0-23
volatile uint8_t alarmDay = 0; // The day of the alarm
volatile boolean alarmSet; // Whether the alarm is set or not
#define CLOCK_MODE_ADDR  0 // Address of where mode is stored in the NVRAM
#define ALARM_MIN_ADDR   1 // Address of where alarm minute is stored in the NVRAM
#define ALARM_HR_ADDR    2 // Address of where alarm hour is stored in the NVRAM
#define ALARM_SET_ADDR   3 // Address of where alarm state is stored in the NVRAM
#define ALARM_MODE_ADDR  4 // Address of where the alarm mode is stored in the NVRAM
#define LED_OFFSET_ADDR  5 // Address of where the LED offset is stored in the NVRAM
boolean alarmTrig = false; // Whether the alarm has been triggered or not
uint32_t alarmTrigTime; // Milli seconds since the alarm was triggered
boolean countDown = false;
long countDownTime = 0;
long currentCountDown = 0;
long startCountDown;
int countDownMin;
int countDownSec;
int countDownFlash;
int demoIntro = 0;
volatile int j = 0;  // LED position in fast transition effects
long currentMillis;
long previousMillis = 0;
float brightFadeRad;
volatile uint8_t star = 0;
volatile float starBlinks;
volatile bool isBuzzerActive;  // Flag to avoid repetitive buzzer calls
volatile int8_t led_offset;  // Allows rotate clock by 90 segrees left/right 

volatile int state = 0; // Variable of the state of the clock, with the following defined states 
#define STATE_CLOCK 0
#define STATE_ALARM 1
#define STATE_SET_ALARM_HR 2
#define STATE_SET_ALARM_MIN 3
#define STATE_SET_CLOCK_HR 4
#define STATE_SET_CLOCK_MIN 5
#define STATE_SET_CLOCK_SEC 6
#define STATE_SET_CLOCK_UP 7
#define STATE_COUNTDOWN 8
#define STATE_DEMO 9
volatile uint8_t clockMode; // Variable of the display mode of the clock
#define CLOCK_MODE_MAX 8 // Change this when new modes are added. This is so selecting modes can go back beyond.
volatile uint8_t alarmMode; // Variable of the alarm display mode
#define ALARM_MODE_MAX 3

Bounce menuBouncer = Bounce(PIN_MENU,30); // Instantiate a Bounce object with a 50 millisecond debounce time for the menu button
boolean menuButton = false; 
boolean menuPressed = false;
boolean menuReleased = false;
volatile int16_t rotaryMove = 0;
volatile boolean countTime = false;
long menuTimePressed;
volatile long lastRotary;
int pendulumPos;

#define R_SHIFT	0
#define G_SHIFT	20
#define B_SHIFT	40
uint8_t color_intensity [] = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 5, 
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                              5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100};

void setup() {
  // Set up all pins
  pinMode(PIN_MENU, INPUT_PULLUP);     // Uses the internal 20k pull up resistor. Pre Arduino_v.1.0.1 need to be "digitalWrite(PIN_MENU,HIGH);pinMode(PIN_MENU,INPUT);"
    
  // Start LEDs
  LEDS.addLeds<WS2812B, PIN_LEDS, GRB>(leds, NUM_LEDS); // Structure of the LED data. I have changed to from rgb to grb, as using an alternative LED strip. Test & change these if you're getting different colours. 
  
  // Start RTC
  Wire.begin(); // Starts the Wire library allows I2C communication to the Real Time Clock
  RTC.begin(); // Starts communications to the RTC
  
  Serial.begin(9600); // Starts the serial communications

  // Uncomment to reset all the NVRAM addresses. You will have to comment again and reload, otherwise it will not save anything each time power is cycled
  // write a 0 to all 512 uint8_ts of the NVRAM
//  for (int i = 0; i < 512; i++)
//  {RTC.writenvram(i, 0);}

  // Load any saved setting since power off, such as mode & alarm time  
  clockMode = RTC.readnvram(CLOCK_MODE_ADDR); 
  alarmMin = RTC.readnvram(ALARM_MIN_ADDR); 
  alarmHour = RTC.readnvram(ALARM_HR_ADDR); 
  alarmSet = RTC.readnvram(ALARM_SET_ADDR); 
  alarmMode = RTC.readnvram(ALARM_MODE_ADDR);
  led_offset = RTC.readnvram(LED_OFFSET_ADDR);
  // Sanity check for virgin device
  clockMode = (clockMode >= CLOCK_MODE_MAX)?0:clockMode;
  alarmMin = (alarmMin >= 60)?0:alarmMin;
  alarmHour = (alarmHour >= 24)?0:alarmHour;
  alarmSet = (alarmSet > 1)?false:alarmSet;
  alarmMode = (alarmMode >= ALARM_MODE_MAX)?0:alarmMode;
  led_offset = (led_offset == LED_OFFSET_L || led_offset == LED_OFFSET_R)?led_offset:0;
  // Write sanitized data back to NVRAM for further proper boot.
  RTC.writenvram(CLOCK_MODE_ADDR, clockMode); 
  RTC.writenvram(ALARM_MIN_ADDR, alarmMin); 
  RTC.writenvram(ALARM_HR_ADDR, alarmHour); 
  RTC.writenvram(ALARM_SET_ADDR, alarmSet); 
  RTC.writenvram(ALARM_MODE_ADDR, alarmMode);
  RTC.writenvram(LED_OFFSET_ADDR, led_offset);
  rotary1.write(0);  // Set non-interrupt mode to rotary
  state = STATE_CLOCK;

  // Print all the saved NVRAM data to Serial
  Serial.print("Mode is "); Serial.println(clockMode);
  Serial.print("Alarm Hour is "); Serial.println(alarmHour);
  Serial.print("Alarm Min is "); Serial.println(alarmMin);
  Serial.print("Alarm is set "); Serial.println(alarmSet);
  Serial.print("Alarm Mode is "); Serial.println(alarmMode);
  Serial.print("LED offset is "); Serial.println(led_offset);
  printDateTime();

  EasyBuzzer.setPin(PIN_BUZZER);
  clearBuzzer();
}


void loop() {
  DateTime now = RTC.now(); // Fetches the time from RTC
  EasyBuzzer.update();

  // Check for any button presses and action accordingly
  menuButton = menuBouncer.update();  // Update the debouncer for the menu button and saves state to menuButton
  rotary1Pos = rotary1.read(); // Checks the rotary position
  if (rotary1Pos != 0) {
    if (millis() - lastRotary >= ROTARY_SET_TIME_MS) {
      rotaryMove = (rotary1Pos < 0)?-1:1;  // Limit moves to single step.
      rotary1.write(0);
      lastRotary = millis();
    } else {  // Reset position if no valid move detected.
      rotary1Pos = 0;
      rotary1.write(0);
    }
  }
  if (menuButton == true || rotaryMove != 0 || countTime == true) {buttonCheck(menuBouncer,now);}

  // clear LED array
  memset(leds, 0, NUM_LEDS * 3);
  
  // Check alarm and trigger if the time matches
  if (alarmSet == true && alarmDay != now.day()) { // The alarmDay statement ensures it is a newly set alarm or repeat from previous day, not within the minute of an alarm cancel.
    if (alarmTrig == false) {alarm(now);}
    else {alarmDisplay();}
  }
 // Check the Countdown Timer
  if (countDown == true) {
    currentCountDown = countDownTime + startCountDown - now.unixtime();
    if ( currentCountDown <= 0) state = STATE_COUNTDOWN;
  } 
  // Set the time LED's
  if (state == STATE_SET_CLOCK_HR || state == STATE_SET_CLOCK_MIN || state == STATE_SET_CLOCK_SEC || state == STATE_SET_CLOCK_UP) {setClockDisplay(now);}
  else if (state == STATE_ALARM || state == STATE_SET_ALARM_HR || state == STATE_SET_ALARM_MIN) {setAlarmDisplay();}
  else if (state == STATE_COUNTDOWN) {countDownDisplay(now);}
  else if (state == STATE_DEMO) {runDemo(now);}
  else {timeDisplay(now);}

  // Update LEDs
  LEDS.show();
}

void printDateTime() {
  DateTime now = RTC.now();      
  Serial.print("Hour time is... "); Serial.println(now.hour());
  Serial.print("Min time is... "); Serial.println(now.minute());
  Serial.print("Sec time is... "); Serial.println(now.second());
  Serial.print("Year is... "); Serial.println(now.year());
  Serial.print("Month is... "); Serial.println(now.month());
  Serial.print("Day is... "); Serial.println(now.day());
}

void buttonCheck(Bounce menuBouncer, DateTime now) {
  countTime = ! menuBouncer.read();
  if (countTime) { // otherwise will menuBouncer.duration will 
    menuTimePressed = menuBouncer.duration();
    if (menuTimePressed >= (HOLD_TIME_MS - 100) && menuTimePressed <= HOLD_TIME_MS) { // long click
      // blink display while button is pressed to indicate "entered adjust mode"
      clearLEDs();
      LEDS.show();
      delay(100);
    }
  }
  menuReleased = menuBouncer.risingEdge();
  // Stop alarm on click or rotate
  if (alarmTrig == true) {
    alarmTrig = false;
    clearBuzzer();
    alarmDay = now.day(); // When the alarm is cancelled it will not display until next day. As without it, it would start again if within a minute, or completely turn off the alarm.
    delay(300); // let time for the button to be released
    return; // This return exits the buttonCheck function, so no actions are performs
  }  
  switch (state) {
    case STATE_CLOCK: // State 0
      // Progress next mode from current mode.
      if(rotaryMove != 0) {
        clockMode = (clockMode + rotaryMove) % CLOCK_MODE_MAX; // Never exceed CLOCK_MODE_MAX
        RTC.writenvram(CLOCK_MODE_ADDR, clockMode);
        rotaryMove = 0;
      } else if(menuReleased == true) {
        if (menuTimePressed <= HOLD_TIME_MS) {
          state = STATE_ALARM; 
          newSecTime = millis();
        } else state = STATE_SET_CLOCK_HR;
      }
      break;
    case STATE_ALARM: // State 1
      if (rotaryMove != 0) {
        alarmMode = (alarmMode + rotaryMove) % (ALARM_MODE_MAX + 1);  // Never exceed CLOCK_MODE_MAX but 0 is alarm off
        if (alarmMode == 0) {alarmSet = 0;}
        else {alarmSet = 1;}
      }          
      Serial.print("alarmSet is "); Serial.println(alarmSet);            
      Serial.print("alarmMode is ");  Serial.println(alarmMode);
      RTC.writenvram(ALARM_SET_ADDR, alarmSet);
      RTC.writenvram(ALARM_MODE_ADDR, alarmMode);
      rotaryMove = 0;
      alarmTrig = false;
      if (menuReleased == true) {
        if (menuTimePressed <= HOLD_TIME_MS) {state = STATE_COUNTDOWN; j = 0;}// if displaying the alarm time, menu button is pressed & released, then clock is displayed
        else {state = STATE_SET_ALARM_HR;} // if displaying the alarm time, menu button is held & released, then alarm hour can be set
      }
      break;
    case STATE_SET_ALARM_HR: // State 2
      if (menuReleased == true) {
        state = STATE_SET_ALARM_MIN;
      } else {
        alarmHour = (alarmHour + rotaryMove < 0) ? 23 : (alarmHour + rotaryMove) % 24;
      }
      RTC.writenvram(ALARM_HR_ADDR, alarmHour);
      rotaryMove = 0;
      break;
    case STATE_SET_ALARM_MIN: // State 3
      if (menuReleased == true) {
        state = STATE_ALARM;
        alarmDay = 0;
        newSecTime = millis();
      } else {
        alarmMin = (alarmMin + rotaryMove < 0) ? 59 : (alarmMin + rotaryMove) % 60;
      }
      RTC.writenvram(ALARM_MIN_ADDR, alarmMin);
      rotaryMove = 0;
      break;
    case STATE_SET_CLOCK_HR: // State 4
      if (menuReleased == true) {state = STATE_SET_CLOCK_MIN;}
      else if (rotaryMove != 0) {
        int h = (now.hour() + rotaryMove < 0) ? 23 : (now.hour() + rotaryMove) % 24;
        RTC.adjust(DateTime(now.year(), now.month(), now.day(), h, now.minute(), now.second()));
        rotaryMove = 0;
      }
      break;
    case STATE_SET_CLOCK_MIN: // State 5
      if (menuReleased == true) {state = STATE_SET_CLOCK_SEC;}
      else if (rotaryMove != 0) {
        int m = (now.minute() + rotaryMove < 0) ? 59 : (now.minute() + rotaryMove) % 60;
        RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), m, now.second()));
        rotaryMove = 0;
      }
      break;
    case STATE_SET_CLOCK_SEC: // State 6
      if (menuReleased == true) {state = STATE_SET_CLOCK_UP;}
      else if (rotaryMove != 0) {
        int s = (now.second() + rotaryMove < 0) ? 59 : (now.second() + rotaryMove) % 60;
        RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), s));
        rotaryMove = 0;
      }
      break;
    case STATE_SET_CLOCK_UP:
      if (menuReleased == true) {state = STATE_CLOCK;}
      else if (rotaryMove != 0) {
        led_offset = (led_offset == LED_OFFSET_L)?LED_OFFSET_R:LED_OFFSET_L;
        RTC.writenvram(LED_OFFSET_ADDR, led_offset);
        rotaryMove = 0;
      }      
      break;
    case STATE_COUNTDOWN: // State 8
      if(menuReleased == true) {  // Count down or switch to non-countdown mode if finished counting. 
        if (menuTimePressed <= HOLD_TIME_MS) {
          // Timer on + button pressed and released quickly ==> stop countdown.
          if (countDown) {
            countDown = false; 
            countDownTime = 0; 
            currentCountDown = 0; 
            clearBuzzer();
          }
          // Timer off, there is time + button pressed and released quickly ==> start countdown.
          else if (countDown == false && countDownTime > 0) {countDown = true; startCountDown = now.unixtime();}
          // Timer on + there's time OR Timer off + time gone + button is pressed & released, then demo State is displayed 
          else {state = STATE_DEMO; demoIntro = 1; j = 0;}
        } else { // if displaying the countdown + long click => the count down is reset
          countDown = false; countDownTime = 0; 
          currentCountDown = 0; j = 0; 
        } 
      // Button not pressed/released + there is rotary move ==> set timer +/- minutes
      } else if (rotaryMove != 0) {
        // Timer on, time is gone + rotated ==> stop countdown, stop buzzer.
        if (countDown && currentCountDown <= 0) {
          countDownTime = 0; 
          currentCountDown = 0; 
          clearBuzzer();
        } else { // Timer is off => set time silently.
          countDown = false;
          EasyBuzzer.stopBeep();
          // Convert currentCountDown secs to mins, add change, fix by module, convert back to secs.
          countDownTime = ((currentCountDown/60 + rotaryMove) % 60) * 60; 
          // Go full clock (60 mins) if user rotates left from 0 minutes. (save rotation for ling timer).
          if (countDownTime < 0) countDownTime = 3600;
        }
      }
      rotaryMove = 0;
      break;
    case STATE_DEMO: // State 9
      if(menuReleased == true) {state = STATE_CLOCK; }  // if displaying the demo, menu button pressed then the clock will display and restore to the mode before demo started
      break;
  }
  if (state == STATE_SET_CLOCK_HR || state == STATE_SET_CLOCK_MIN || state == STATE_SET_CLOCK_SEC) printDateTime();
  if (menuReleased || rotaryMove !=0) {countTime = false;}
  Serial.print("Mode is ");  Serial.println(clockMode);
  Serial.print("State is ");  Serial.println(state);
}

void setAlarmDisplay() {
  for (int i = 0; i < NUM_LEDS; i += 5) {
    // Apply to every 5th LED (5-minute ticks)
    leds[i].r = 100;
    leds[i].g = 100;
    leds[i].b = 100;
  }

  if (alarmSet == 0) {
    for (int i = 0; i < NUM_LEDS; i += 5) { // Sets background to red, to state that alarm IS NOT set
      // Apply to every 5th LED (5-minute ticks)
      leds[i].r = 20;
      leds[i].g = 0;
      leds[i].b = 0;
    }     
  } else {
    for (int i = 0; i < NUM_LEDS; i += 5) { // Sets background to green, to state that alarm IS set
      // Apply to every 5th LED (5-minute ticks)
      leds[i].r = 0;
      leds[i].g = 20;
      leds[i].b = 0;
    }     
  }
  if (alarmHour <= 11) {
    leds[(alarmHour*5+led_offset)%NUM_LEDS].r = 255;
  } else {
    leds[((alarmHour - 12)*5+led_offset-1)%NUM_LEDS].r = 50;    
    leds[((alarmHour - 12)*5+led_offset)%NUM_LEDS].r = 255;
    leds[((alarmHour - 12)*5+led_offset+1)%NUM_LEDS].r = 50;
  }
  leds[(alarmMin+led_offset)%NUM_LEDS].g = 100;
  flashTime = millis();
  // Turn off hourly ticks periodically to show flashing.
  if (state == STATE_SET_ALARM_HR && flashTime%300 >= 150) {
    leds[(((alarmHour%12)*5)+led_offset-1)%NUM_LEDS].r = 0;   
    leds[(((alarmHour%12)*5)+led_offset)%NUM_LEDS].r = 0;
    leds[(((alarmHour%12)*5)+led_offset+1)%NUM_LEDS].r = 0; 
  }
  if (state == STATE_SET_ALARM_MIN && flashTime%300 >= 150) {
    leds[(alarmMin+led_offset)%NUM_LEDS].g = 0;
  }
  leds[(alarmMode+led_offset)%NUM_LEDS].b = 255;
}

void setClockDisplay(DateTime now) {
  for (int i = 0; i < NUM_LEDS; i += 5) {
    // Apply to every 5th LED (5-minute ticks)
    leds[i].r = 10;
    leds[i].g = 10;
    leds[i].b = 10;
  } 
  if (now.hour() <= 11) {
    leds[(now.hour()*5+led_offset)%NUM_LEDS].r = 255;
  } else {
    leds[((now.hour() - 12)*5+led_offset-1)%NUM_LEDS].r = 50;
    leds[((now.hour() - 12)*5+led_offset)%NUM_LEDS].r = 255;   
    leds[((now.hour() - 12)*5+led_offset+1)%NUM_LEDS].r = 50;
  }
  flashTime = millis();
  if (state == STATE_SET_CLOCK_HR && flashTime%300 >= 150) {
    leds[((now.hour()%12)*5+led_offset-1)%NUM_LEDS].r = 0;   
    leds[((now.hour()%12)*5+led_offset)%NUM_LEDS].r = 0;
    leds[((now.hour()%12)*5+led_offset+1)%NUM_LEDS].r = 0; 
  }
  if (state == STATE_SET_CLOCK_MIN && flashTime%300 >= 150) {
    leds[(now.minute()+led_offset)%NUM_LEDS].g = 0;
  } else {leds[(now.minute()+led_offset)%NUM_LEDS].g = 255;}
  if (state == STATE_SET_CLOCK_SEC && flashTime%300 >= 150) {
    leds[(now.second()+led_offset)%NUM_LEDS].b = 0;
  } else {leds[(now.second()+led_offset)%NUM_LEDS].b = 255;}
}

// Signal buzzer is ready for next action.
void clearBuzzer() {
  EasyBuzzer.stopBeep();
  isBuzzerActive = false;
  // Stop triggers, return to clock mode.
  state = STATE_CLOCK;
  countDown = false;
}

// Enable buzzer sequence only once on reqest until buzzer operation is cleared. Avoid retriggering buzzer.
void runBuzzer(int mode) {
  if (isBuzzerActive) return;
  if (TIMER == mode) {
    EasyBuzzer.beep(BEEP_TONE, 300, 500, 2, 1000, 5, clearBuzzer);  // For end of timer beep 5 double beeps  
  } else {
    EasyBuzzer.beep(BEEP_TONE, 300, 200, 3, 1000, 0); // , callback  // For alarm beep 3 beeps endlessly (until rotary rotate)  
  }
  isBuzzerActive = true;
}
// Check if alarm is active and if is it time for the alarm to trigger
void alarm(DateTime now) {
  if ((alarmMin == now.minute()%60) && (alarmHour == now.hour()%24)) { //check if the time is the same to trigger alarm
    alarmTrig = true;
    alarmTrigTime = millis();
  }
}

void alarmDisplay() {
  runBuzzer(ALARM);
  switch (alarmMode) {
    case 1:
      // set all LEDs to a dim white: avoid overcurrent
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].r = 20;
        leds[i].g = 20;
        leds[i].b = 20;
      }
      break;
    case 2:
      int8_t LEDPosition = (millis() - alarmTrigTime)/300;
      int8_t reverseLEDPosition = NUM_LEDS - LEDPosition;
      // Add here calculation of 1st LED, Last LED and Middle LED
      if (LEDPosition >= 0 && LEDPosition <= (NUM_LEDS/2-1)) {
        for (int i = 0; i < LEDPosition; i++) {
          leds[(i+led_offset)%NUM_LEDS].r = 5;
          leds[(i+led_offset)%NUM_LEDS].g = 5;
          leds[(i+led_offset)%NUM_LEDS].b = 5;
        }
      } else {
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[(i+led_offset)%NUM_LEDS].r = 5;
          leds[(i+led_offset)%NUM_LEDS].g = 5;
          leds[(i+led_offset)%NUM_LEDS].b = 5;
        }           
      } 
      if (reverseLEDPosition <= (NUM_LEDS-1) && reverseLEDPosition >= (NUM_LEDS/2+1)) {
        for (int i = NUM_LEDS-1; i > reverseLEDPosition; i--) {
          leds[(i+led_offset)%NUM_LEDS].r = 5;
          leds[(i+led_offset)%NUM_LEDS].g = 5;
          leds[(i+led_offset)%NUM_LEDS].b = 5;
        }              
      }
      break;
    case 3:
      brightFadeRad = (millis() - alarmTrigTime)/FADE_TIME_MS; // Divided by the time period of the fade up.
      if (millis() > alarmTrigTime + FADE_TIME_MS) brightness = 255;
      else brightness = (int)(255.0*(1.0+sin(1.57*brightFadeRad-1.57)));
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].r = brightness;
        leds[i].g = brightness;
        leds[i].b = brightness;
      }
      break;
  }
}

void countDownDisplay(DateTime now) {
  flashTime = millis();
  if (countDown == true) {
    // Counting down
    currentCountDown = countDownTime + startCountDown - now.unixtime();
    if (currentCountDown > 0) {
      // Time is not gone yet, decrement lights
      countDownMin = currentCountDown / 60;
      countDownSec = currentCountDown%60 * 2; // Range 0-120 to create brightness less than 240
      for (int i = 0; i < countDownMin; i++) {leds[(i+led_offset+1)%NUM_LEDS].b = 240;} // Set a blue LED for each complete minute that is remaining 
      leds[(countDownMin+led_offset+1)%NUM_LEDS].b = countDownSec; // Display the remaining secconds of the current minute as its brightness      
    } else {
      // Time is gone, flash and beep.
      countDownFlash = now.unixtime()%2;
      runBuzzer(TIMER);
      if (countDownFlash == 0) {
        clearLEDs();
      } else {
        for (int i = 0; i < NUM_LEDS; i++) { // Set the background as all blue
          leds[i] = CRGB::Blue;
        }
      }
    }
  } else {
    // Setting mode
    currentCountDown = countDownTime;
    if (countDownTime == 0) {
      currentMillis = millis();
      clearLEDs();
      switch (demoIntro) {
        case 0:
          for (int i = 0; i < j; i++) {leds[(i+led_offset+1)%NUM_LEDS].b = 20;}
          if (currentMillis - previousMillis > TIME_INTERVAL) {j++; previousMillis = currentMillis;}
          if (j == NUM_LEDS) {demoIntro = 1;}
          break;
        case 1:
          for (int i = 0; i < j; i++) {leds[(i+led_offset+1)%NUM_LEDS].b = 20;}
          if (currentMillis - previousMillis > TIME_INTERVAL) {j--; previousMillis = currentMillis;}
          if (j < 0) {demoIntro = 0;}
          break;
      }
    } else if (countDownTime > 0 && flashTime%300 >= 150) {
      countDownMin = currentCountDown / 60;
      for (int i = 0; i < countDownMin; i++) {leds[(i+led_offset+1)%NUM_LEDS].b = 255;} // Set a blue LED for each complete minute that is remaining
    }
  }
}

void runDemo(DateTime now) {
  currentDemoTime = now.unixtime();
  currentMillis = millis();
  clearLEDs();
  switch (demoIntro) {
    case 0:
      timeDisplay(now);
      if (currentDemoTime - previousDemoTime > DEMO_TIME_S) {previousDemoTime = currentDemoTime;}
      break;
    case 1:
      for (int i = 0; i < j; i++) {leds[i].r = 255;}
      if (currentMillis - previousMillis > TIME_INTERVAL) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 2:
      for (int i = j; i < NUM_LEDS; i++) {leds[i].r = 255;}
      if (currentMillis - previousMillis > TIME_INTERVAL) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 3:
      for (int i = 0; i < j; i++) {leds[i].g = 255;}
      if (currentMillis - previousMillis > TIME_INTERVAL) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 4:
      for (int i = j; i < NUM_LEDS; i++) {leds[i].g = 255;}
      if (currentMillis - previousMillis > TIME_INTERVAL) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 5:
      for (int i = 0; i < j; i++) {leds[i].b = 255;}
      if (currentMillis - previousMillis > TIME_INTERVAL) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 6:
      for (int i = j; i < NUM_LEDS; i++) {leds[i].b = 255;}
      if (currentMillis - previousMillis > TIME_INTERVAL) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 7:
      for (int i = 0; i < j; i++) {leds[i] = CRGB::White;}
      if (currentMillis - previousMillis > TIME_INTERVAL) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 8:
      for (int i = j; i < NUM_LEDS; i++) {leds[i] = CRGB::White;}
      if (currentMillis - previousMillis > TIME_INTERVAL) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 9:
      rainbow();
      // if (currentMillis - previousMillis > TIME_INTERVAL) {j++; previousMillis = currentMillis;}
      if (currentMillis - previousMillis > TIME_INTERVAL * 5000) {previousMillis = currentMillis; demoIntro = 1; j = 0; }
      break;
  }
}

void clearLEDs() {    
  for (int i = 0; i < NUM_LEDS; i++) { // Set all the LEDs to off
    leds[i] = CRGB::Black;
  }
}

void timeDisplay(DateTime now) { 
  switch (clockMode) {
    case 0:
      minimalClock(now);
      break;
    case 1:
      basicClock(now);
      break;
    case 2:
      smoothSecond(now);
      break;
    case 3:
      outlineClock(now);
      break;
    case 4:
      minimalMilliSec(now);
      break;
    case 5:
      simplePendulum(now);
      break;
    case 6:
      breathingClock(now);
      break;
    case 7:
      starryNightClock(now);
      break;
    default: // Keep this here and add more timeDisplay modes as defined cases.
      clockMode = RTC.readnvram(CLOCK_MODE_ADDR);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////
//   CLOCK DISPLAY MODES
// Add any new display mode functions here. Then add to the "void timeDisplay(DateTime now)" function.
// Add each of the new display mode functions as a new "case", leaving default last.
////////////////////////////////////////////////////////////////////////////////////////////

// Just 3 LEDs on.
void minimalClock(DateTime now) {
  unsigned char hourPos = (now.hour()%12)*5;
  leds[(hourPos+led_offset)%NUM_LEDS].r = 255;
  leds[(now.minute()+led_offset)%NUM_LEDS].g = 255;
  leds[(now.second()+led_offset)%NUM_LEDS].b = 255;
}

// # Red LEDs for hours, 1 LED per min and sec.
void basicClock(DateTime now) {
  unsigned char hourPos = (now.hour()%12)*5 + (now.minute()+6)/12;
  leds[(hourPos+led_offset-1)%NUM_LEDS].r = 50;
  leds[(hourPos+led_offset)%NUM_LEDS] = CRGB::Red;
  leds[(hourPos+led_offset+1)%NUM_LEDS].r = 50;
  // Mix colors if the same LED is chosen
  leds[(now.minute()+led_offset)%NUM_LEDS].g = 255;
  leds[(now.second()+led_offset)%NUM_LEDS].b = 255;
}

// Second hand flowing from sec-to-sec + Basic clock
void smoothSecond(DateTime now) {
  basicClock(now);
  if (now.second()!=old.second()) {
    old = now;
    cyclesPerSec = millis() - newSecTime;
    cyclesPerSecFloat = (float) cyclesPerSec;
    newSecTime = millis();      
  } 
  // set hour, min & sec LEDs
  fracOfSec = (millis() - newSecTime)/cyclesPerSecFloat;  // This divides by 733, but should be 1000 and not sure why???
  if (subSeconds < cyclesPerSec) { brightness = 50.0*(1.0+sin((3.14*fracOfSec)-1.57)); }
  leds[(now.second()+led_offset)%NUM_LEDS].b = brightness;
  leds[(now.second()+led_offset-1)%NUM_LEDS].b = 100 - brightness;
}

// Constant lit 5-minute ticks + Basic clock
void outlineClock(DateTime now) {
  for (int i = 0; i < NUM_LEDS; i += 5) {
    // Apply to every 5th LED (5-minute ticks)
    leds[i].r = 100;
    leds[i].g = 100;
    leds[i].b = 100;
  }
  basicClock(now);
}

void starryNightClock(DateTime now) {
  basicClock(now);
  if (now.second()!=old.second()) {
    star = random8(NUM_LEDS);           // Choose star
    starBlinks = (float)(random8(1, 4) * 8);   // Choose times of sparkles
    old = now;
  } 
  float m = (float) (millis() % 2000) / 3000.0;
  brightness = (2.0-m)*15.0*(1.0+sin(m*starBlinks-0.7));
  brightness = min(brightness, 100);  // cut numbers > 100
  brightness = (brightness < 15)?0:brightness;  // cut numbers < 15
  leds[star].r = brightness;
  leds[star].g = brightness;
  leds[star].b = brightness;
}

// Running white light over clock round. Full round in 1 second.
void minimalMilliSec(DateTime now) {
  if (now.second()!=old.second()) {
    old = now;
    cyclesPerSec = (millis() - newSecTime);
    newSecTime = millis();
  } 
  // set hour, min & sec LEDs
  unsigned char hourPos = (now.hour()%12)*5 + (now.minute()+6)/12;
  subSeconds = (((millis() - newSecTime)*60)/cyclesPerSec)%60;  // This divides by 733, but should be 1000 and not sure why???
  // Millisec lights are set first, so hour/min/sec lights override and don't flicker as millisec passes
  leds[(subSeconds+led_offset)%NUM_LEDS].r = 50;
  leds[(subSeconds+led_offset)%NUM_LEDS].g = 50;
  leds[(subSeconds+led_offset)%NUM_LEDS].b = 50;
  // The colours are set last, so if on same LED mixed colours are created
  leds[(hourPos+led_offset-1)%NUM_LEDS].r = 50;   
  leds[(hourPos+led_offset)%NUM_LEDS].r = 255;
  leds[(hourPos+led_offset+1)%NUM_LEDS].r = 50;
  leds[(now.minute()+led_offset)%NUM_LEDS].g = 255;
  leds[(now.second()+led_offset)%NUM_LEDS].b = 255;
}

// Pendulum will be at the bottom and left for one second and right for one second
void simplePendulum(DateTime now) {
  basicClock(now);
  if (now.second()!=old.second()) {
    old = now;
    cyclesPerSec = millis() - newSecTime;
    cyclesPerSecFloat = (float) cyclesPerSec;
    newSecTime = millis();
    swingBack = -swingBack;
  } 
  fracOfSec = (millis() - newSecTime)/cyclesPerSecFloat;  // This divides by 733, but should be 1000 and not sure why???
  if (subSeconds < cyclesPerSec) {
    pendulumPos = (NUM_LEDS/2 - led_offset - 3) % NUM_LEDS + 3.4*(1.0+sin((3.14*fracOfSec)+swingBack));
    pendulumPos = (pendulumPos < 0)?-pendulumPos:pendulumPos;
  }
  // Pendulum lights are set first, so hour/min/sec lights override and don't flicker as millisec passes
  leds[pendulumPos].r = 100;
  leds[pendulumPos].g = 100;
  leds[pendulumPos].b = 100;
}

void breathingClock(DateTime now) {
  brightness = 30.0*(1.0+sin((3.14*millis()/2000.0)-1.57)) + 2;
  for (int i = 0; i < NUM_LEDS; i += 5) {
    // Apply to every 5th LED (5-minute ticks)
    leds[i].r = brightness;
    leds[i].g = brightness;
    leds[i].b = brightness;
  }
  basicClock(now);
}

void rainbow() {
  // Set/progress initial position for the rainbow.
  star++;
  star = star % NUM_LEDS;

  // Fill the rainbow
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].r = color_intensity[(star + i + R_SHIFT) % NUM_LEDS];
    leds[i].g = color_intensity[(star + i + G_SHIFT) % NUM_LEDS];
    leds[i].b = color_intensity[(star + i + B_SHIFT) % NUM_LEDS];
  }
  delay(10);
}

