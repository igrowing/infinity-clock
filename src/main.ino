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

RTC_DS1307 RTC;     // Establishes the chipset of the Real Time Clock

#define PIN_LEDS    A0    // Pin used for the data to the LED strip
#define PIN_MENU    PIN4  // Pin used for the menu button (green stripe)
#define NUM_LEDS    60    // Number of LEDs in strip
#define LED_OFFSET  30    // Adjust by LED position/shift in the circle
#define HOLD_TIME_MS 1500
#define DEMO_TIME_S 12 // seconds
#define ROTARY_SET_TIME_MS 300

struct CRGB leds[NUM_LEDS];  // Setting up the LED strip
Encoder rotary1(PIN2, PIN3); // Setting up the Rotary Encoder

DateTime old; // Variable to compare new and old time, to see if it has moved on.
int rotary1Pos  = 0;
int subSeconds; // 60th's of a second
int secondBrightness;
int secondBrightness2;
int breathBrightness;
long newSecTime; // Variable to record when a new second starts, allowing to create milli seconds
long oldSecTime;
long flashTime; //
long breathCycleTime;
int cyclesPerSec;
float cyclesPerSecFloat; // So can be used as a float in calcs
float fracOfSec;
float breathFracOfSec;
boolean demo;
long previousDemoTime;
long currentDemoTime;
boolean swingBack = false;

volatile int timeHour;
volatile int timeMin;
volatile int timeSec;
volatile uint8_t alarmMin; // The minute of the alarm  
volatile uint8_t alarmHour; // The hour of the alarm 0-23
volatile uint8_t alarmDay = 0; // The day of the alarm
volatile boolean alarmSet; // Whether the alarm is set or not
#define CLOCK_MODE_ADDR  0 // Address of where mode is stored in the EEPROM
#define ALARM_MIN_ADDR   1 // Address of where alarm minute is stored in the EEPROM
#define ALARM_HR_ADDR    2 // Address of where alarm hour is stored in the EEPROM
#define ALARM_SET_ADDR   3 // Address of where alarm state is stored in the EEPROM
#define ALARM_MODE_ADDR  4 // Address of where the alarm mode is stored in the EEPROM
boolean alarmTrig = false; // Whether the alarm has been triggered or not
long alarmTrigTime; // Milli seconds since the alarm was triggered
boolean countDown = false;
long countDownTime = 0;
long currentCountDown = 0;
long startCountDown;
int countDownMin;
int countDownSec;
int countDownFlash;
int demoIntro = 0;
volatile int j = 0;
long timeInterval = 5;
long currentMillis;
long previousMillis = 0;
float LEDBrightness = 0;
float fadeTime;
float brightFadeRad;

volatile int state = 0; // Variable of the state of the clock, with the following defined states 
#define STATE_CLOCK 0
#define STATE_ALARM 1
#define STATE_SET_ALARM_HR 2
#define STATE_SET_ALARM_MIN 3
#define STATE_SET_CLOCK_HR 4
#define STATE_SET_CLOCK_MIN 5
#define STATE_SET_CLOCK_SEC 6
#define STATE_COUNTDOWN 7
#define STATE_DEMO 8
volatile uint8_t clockMode; // Variable of the display mode of the clock
#define CLOCK_MODE_MAX 7 // Change this when new modes are added. This is so selecting modes can go back beyond.
volatile uint8_t alarmMode; // Variable of the alarm display mode
#define ALARM_MODE_MAX 3

Bounce menuBouncer = Bounce(PIN_MENU,20); // Instantiate a Bounce object with a 50 millisecond debounce time for the menu button
boolean menuButton = false; 
boolean menuPressed = false;
boolean menuReleased = false;
volatile uint8_t rotaryMove = 0;
volatile boolean countTime = false;
long menuTimePressed;
volatile long lastRotary;

int LEDPosition;
int reverseLEDPosition;
int pendulumPos;
int odd;

void setup() {
  // Set up all pins
  pinMode(PIN_MENU, INPUT_PULLUP);     // Uses the internal 20k pull up resistor. Pre Arduino_v.1.0.1 need to be "digitalWrite(PIN_MENU,HIGH);pinMode(PIN_MENU,INPUT);"
    
  // Start LEDs
  LEDS.addLeds<WS2812B, PIN_LEDS, GRB>(leds, NUM_LEDS); // Structure of the LED data. I have changed to from rgb to grb, as using an alternative LED strip. Test & change these if you're getting different colours. 
  
  // Start RTC
  Wire.begin(); // Starts the Wire library allows I2C communication to the Real Time Clock
  RTC.begin(); // Starts communications to the RTC
  
  Serial.begin(9600); // Starts the serial communications

  // Uncomment to reset all the EEPROM addresses. You will have to comment again and reload, otherwise it will not save anything each time power is cycled
  // write a 0 to all 512 uint8_ts of the EEPROM
//  for (int i = 0; i < 512; i++)
//  {RTC.writenvram(i, 0);}

  // Load any saved setting since power off, such as mode & alarm time  
  clockMode = RTC.readnvram(CLOCK_MODE_ADDR); // The mode will be stored in the address "0" of the EEPROM
  alarmMin = RTC.readnvram(ALARM_MIN_ADDR); // The mode will be stored in the address "1" of the EEPROM
  alarmHour = RTC.readnvram(ALARM_HR_ADDR); // The mode will be stored in the address "2" of the EEPROM
  alarmSet = RTC.readnvram(ALARM_SET_ADDR); // The mode will be stored in the address "2" of the EEPROM
  alarmMode = RTC.readnvram(ALARM_MODE_ADDR);
  // Sanity check for virgin device
  clockMode = (clockMode >= CLOCK_MODE_MAX)?0:clockMode;
  alarmMin = (alarmMin >= 60)?0:alarmMin;
  alarmHour = (alarmHour >= 24)?0:alarmHour;
  alarmSet = (alarmSet > 1)?false:alarmSet;
  alarmMode = (alarmMode >= ALARM_MODE_MAX)?0:alarmMode;
  rotary1.write(0);  // Set non-interrupt mode to rotary

  // Prints all the saved EEPROM data to Serial
  Serial.print("Mode is ");Serial.println(clockMode);
  Serial.print("Alarm Hour is ");Serial.println(alarmHour);
  Serial.print("Alarm Min is ");Serial.println(alarmMin);
  Serial.print("Alarm is set ");Serial.println(alarmSet);
  Serial.print("Alarm Mode is ");Serial.println(alarmMode);

  printDateTime();
}


void loop() {
  DateTime now = RTC.now(); // Fetches the time from RTC
  
  // Check for any button presses and action accordingley
  menuButton = menuBouncer.update();  // Update the debouncer for the menu button and saves state to menuButton
  rotary1Pos = rotary1.read(); // Checks the rotary position
  if (rotary1Pos != 0) {
    if (millis() - lastRotary >= ROTARY_SET_TIME_MS) {
      rotaryMove = (rotary1Pos < 0)?-1:1;
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
  if (state == STATE_SET_CLOCK_HR || state == STATE_SET_CLOCK_MIN || state == STATE_SET_CLOCK_SEC) {setClockDisplay(now);}
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
  countTime = ! menuBouncer.read();  // Does the same as 10 lines below.
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
  if (alarmTrig == true) {
    alarmTrig = false;
    alarmDay = now.day(); // When the alarm is cancelled it will not display until next day. As without it, it would start again if within a minute, or completely turn off the alarm.
    delay(300); // let time for the button to be released
    return; // This return exits the buttonCheck function, so no actions are performs
  }  
  switch (state) {
    case STATE_CLOCK: // State 0
      // Progress next mode from current mode.
      if(rotaryMove != 0) {
        clockMode = (clockMode + rotaryMove) % CLOCK_MODE_MAX; // Never exceed CLOCK_MODE_MAX
        RTC.writenvram(CLOCK_MODE_ADDR, clockMode);  // TODO: no state written if turning rotary left. Move this to timer.
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
        alarmMode = (alarmMode + rotaryMove) % (ALARM_MODE_MAX + 1); // // Never exceed CLOCK_MODE_MAX but 0 is alarm off
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
      if (menuReleased == true) {state = STATE_SET_ALARM_MIN;}
      else if (rotaryMove == 1 && alarmHour >= 23) {alarmHour = 0;}
      else if (rotaryMove == -1 && alarmHour <= 0) {alarmHour = 23;}
      else if (rotaryMove != 0) {alarmHour = alarmHour + rotaryMove;}
      RTC.writenvram(ALARM_HR_ADDR, alarmHour);
      rotaryMove = 0;
      break;
    case STATE_SET_ALARM_MIN: // State 3
      if (menuReleased == true) {
        state = STATE_ALARM;
        alarmDay = 0;
        newSecTime = millis();
      } else if (rotaryMove == 1 && alarmMin >= 59) {alarmMin = 0;}
        else if (rotaryMove == -1 && alarmMin <= 0) {alarmMin = 59;}
        else if (rotaryMove != 0) {alarmMin = alarmMin + rotaryMove;}
      RTC.writenvram(ALARM_MIN_ADDR, alarmMin);
      rotaryMove = 0;
      break;
    case STATE_SET_CLOCK_HR: // State 4
      if (menuReleased == true) {state = STATE_SET_CLOCK_MIN;}
      else if (rotaryMove != 0) {
        RTC.adjust(DateTime(now.year(), now.month(), now.day(), (now.hour() + rotaryMove) % 24, now.minute(), now.second()));
        rotaryMove = 0;
      }
      break;
    case STATE_SET_CLOCK_MIN: // State 5
      if (menuReleased == true) {state = STATE_SET_CLOCK_SEC;}
      else if (rotaryMove != 0) {
        RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), (now.minute() + rotaryMove) % 60, now.second()));
        rotaryMove = 0;
      }
      break;
    case STATE_SET_CLOCK_SEC: // State 6
      if (menuReleased == true) {state = STATE_CLOCK;}
      else if (rotaryMove != 0) {
        RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), (now.second() + rotaryMove) % 60));
        rotaryMove = 0;
      }
      break;
    case STATE_COUNTDOWN: // State 7
      if(menuReleased == true) {  // Count down or switch to non-countdown mode if finished counting. 
        if (menuTimePressed <= HOLD_TIME_MS) {
          if (countDown == true && countDownTime <= 0) {countDown = false; countDownTime = 0; currentCountDown = 0;}
          else if (countDown == false && countDownTime > 0) {countDown = true; startCountDown = now.unixtime();}
          else {state = STATE_DEMO; demoIntro = 1; j = 0;}// if displaying the count down, menu button is pressed & released, then demo State is displayed 
        } else {countDown = false; countDownTime = 0; currentCountDown = 0; j = 0;} // if displaying the clock, menu button is held & released, then the count down is reset
      } else if (rotaryMove == -1 && currentCountDown <= 0) {  // Setting mode for count down
        countDown = false;
        countDownTime = 0;
        currentCountDown = 0;
        demoIntro = 0;          
      } else if (rotaryMove == 1 && currentCountDown >= 3600) {
        countDown = false;
        countDownTime = 3600;           
      } else if (rotaryMove != 0) {
        countDown = false;
        countDownTime = currentCountDown - currentCountDown%60 + rotaryMove*60; // This rounds the count down minute up to the next minute
      }
      rotaryMove = 0;
      break;
    case STATE_DEMO: // State 8
      if(menuReleased == true) {state = STATE_CLOCK; clockMode = RTC.readnvram(CLOCK_MODE_ADDR);} // if displaying the demo, menu button pressed then the clock will display and restore to the mode before demo started
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
    leds[(alarmHour*5+LED_OFFSET)%60].r = 255;
  } else {
    leds[((alarmHour - 12)*5+LED_OFFSET+59)%60].r = 25;    
    leds[((alarmHour - 12)*5+LED_OFFSET)%60].r = 255;
    leds[((alarmHour - 12)*5+LED_OFFSET+1)%60].r = 25;
  }
  leds[(alarmMin+LED_OFFSET)%60].g = 100;
  flashTime = millis();
  if (state == STATE_SET_ALARM_HR && flashTime%300 >= 150) {
    leds[(((alarmHour%12)*5)+LED_OFFSET+59)%60].r = 0;   
    leds[(((alarmHour%12)*5)+LED_OFFSET)%60].r = 0;
    leds[(((alarmHour%12)*5)+LED_OFFSET+1)%60].r = 0; 
  }
  if (state == STATE_SET_ALARM_MIN && flashTime%300 >= 150) {
    leds[(alarmMin+LED_OFFSET)%60].g = 0;
  }
  leds[(alarmMode+LED_OFFSET)%60].b = 255;
}

void setClockDisplay(DateTime now) {
  for (int i = 0; i < NUM_LEDS; i += 5) {
    // Apply to every 5th LED (5-minute ticks)
    leds[i].r = 10;
    leds[i].g = 10;
    leds[i].b = 10;
  } 
  if (now.hour() <= 11) {
    leds[(now.hour()*5+LED_OFFSET)%60].r = 255;
  } else {
    leds[((now.hour() - 12)*5+LED_OFFSET+59)%60].r = 255;
    leds[((now.hour() - 12)*5+LED_OFFSET)%60].r = 255;   
    leds[((now.hour() - 12)*5+LED_OFFSET+1)%60].r = 255;
  }
  flashTime = millis();
  if (state == STATE_SET_CLOCK_HR && flashTime%300 >= 150) {
    leds[(((now.hour()%12)*5)+LED_OFFSET+59)%60].r = 0;   
    leds[((now.hour()%12)*5+LED_OFFSET)%60].r = 0;
    leds[(((now.hour()%12)*5)+LED_OFFSET+1)%60].r = 0; 
  }
  if (state == STATE_SET_CLOCK_MIN && flashTime%300 >= 150) {
    leds[(now.minute()+LED_OFFSET)%60].g = 0;
  } else {leds[(now.minute()+LED_OFFSET)%60].g = 255;}
  if (state == STATE_SET_CLOCK_SEC && flashTime%300 >= 150) {
    leds[(now.second()+LED_OFFSET)%60].b = 0;
  } else {leds[(now.second()+LED_OFFSET)%60].b = 255;}
}

// Check if alarm is active and if is it time for the alarm to trigger
void alarm(DateTime now) {
  if ((alarmMin == now.minute()%60) && (alarmHour == now.hour()%24)) { //check if the time is the same to trigger alarm
    alarmTrig = true;
    alarmTrigTime = millis();
  }
}

void alarmDisplay() {
  switch (alarmMode) {
    case 1:
      // set all LEDs to a dim white
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].r = 100;
        leds[i].g = 100;
        leds[i].b = 100;
      }
      break;
    case 2:
      LEDPosition = ((millis() - alarmTrigTime)/300);
      reverseLEDPosition = 60 - LEDPosition;
      if (LEDPosition >= 0 && LEDPosition <= 29) {
        for (int i = 0; i < LEDPosition; i++) {
          leds[(i+LED_OFFSET)%60].r = 5;
          leds[(i+LED_OFFSET)%60].g = 5;
          leds[(i+LED_OFFSET)%60].b = 5;
        }
      }
      if (reverseLEDPosition <= 59 && reverseLEDPosition >= 31) {
        for (int i = 59; i > reverseLEDPosition; i--) {
          leds[(i+LED_OFFSET)%60].r = 5;
          leds[(i+LED_OFFSET)%60].g = 5;
          leds[(i+LED_OFFSET)%60].b = 5;
        }              
      }
      if (LEDPosition >= 30) {
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[(i+LED_OFFSET)%60].r = 5;
          leds[(i+LED_OFFSET)%60].g = 5;
          leds[(i+LED_OFFSET)%60].b = 5;
        }           
      }            
      break;
    case 3:
      fadeTime = 60000;
      brightFadeRad = (millis() - alarmTrigTime)/fadeTime; // Divided by the time period of the fade up.
      if (millis() > alarmTrigTime + fadeTime) LEDBrightness = 255;
      else LEDBrightness = 255.0*(1.0+sin((1.57*brightFadeRad)-1.57));
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].r = LEDBrightness;
        leds[i].g = LEDBrightness;
        leds[i].b = LEDBrightness;
      }
      break;
  }
}

void countDownDisplay(DateTime now) {
  flashTime = millis();
  if (countDown == true) {
    currentCountDown = countDownTime + startCountDown - now.unixtime();
    if (currentCountDown > 0) {
        countDownMin = currentCountDown / 60;
        countDownSec = currentCountDown%60 * 4; // have multiplied by 4 to create brightness
        for (int i = 0; i < countDownMin; i++) {leds[(i+LED_OFFSET+1)%60].b = 240;} // Set a blue LED for each complete minute that is remaining 
        leds[(countDownMin+LED_OFFSET+1)%60].b = countDownSec; // Display the remaining secconds of the current minute as its brightness      
    } else {
      countDownFlash = now.unixtime()%2;
      if (countDownFlash == 0) {
        for (int i = 0; i < NUM_LEDS; i++) { // Set the background as all off
          leds[i] = CRGB::Black;
        }
      } else {
        for (int i = 0; i < NUM_LEDS; i++) { // Set the background as all blue
          leds[i] = CRGB::Blue;
        }
      }
    }
  } else {
    currentCountDown = countDownTime;
    if (countDownTime == 0) {
      currentMillis = millis();
      clearLEDs();
      switch (demoIntro) {
        case 0:
          for (int i = 0; i < j; i++) {leds[(i+LED_OFFSET+1)%60].b = 20;}
          if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
          if (j == NUM_LEDS) {demoIntro = 1;}
          break;
        case 1:
          for (int i = 0; i < j; i++) {leds[(i+LED_OFFSET+1)%60].b = 20;}
          if (currentMillis - previousMillis > timeInterval) {j--; previousMillis = currentMillis;}
          if (j < 0) {demoIntro = 0;}
          break;
      }
    } else if (countDownTime > 0 && flashTime%300 >= 150) {
      countDownMin = currentCountDown / 60;
      for (int i = 0; i < countDownMin; i++) {leds[(i+LED_OFFSET+1)%60].b = 255;} // Set a blue LED for each complete minute that is remaining
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
      if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 2:
      for (int i = j; i < NUM_LEDS; i++) {leds[i].r = 255;}
      if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 3:
      for (int i = 0; i < j; i++) {leds[i].g = 255;}
      if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 4:
      for (int i = j; i < NUM_LEDS; i++) {leds[i].g = 255;}
      if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 5:
      for (int i = 0; i < j; i++) {leds[i].b = 255;}
      if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 6:
      for (int i = j; i < NUM_LEDS; i++) {leds[i].b = 255;}
      if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 7:
      for (int i = 0; i < j; i++) {leds[i] = CRGB::White;}
      if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {j = 0; demoIntro++;}
      break;
    case 8:
      for (int i = j; i < NUM_LEDS; i++) {leds[i] = CRGB::White;}
      if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
      if (j == NUM_LEDS) {
        demoIntro = 0;
        clockMode = RTC.readnvram(CLOCK_MODE_ADDR);  // Return to kept clock mode
        Serial.print("Mode is "); Serial.println(clockMode);
        Serial.print("State is "); Serial.println(state);
      }
      break;
  }
}

void clearLEDs() {    
  FastLED.clear();  
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
    default: // Keep this here and add more timeDisplay modes as defined cases.
      clockMode = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////
//   CLOCK DISPLAY MODES
// Add any new display mode functions here. Then add to the "void timeDisplay(DateTime now)" function.
// Add each of the new display mode functions as a new "case", leaving default last.
////////////////////////////////////////////////////////////////////////////////////////////

void minimalClock(DateTime now) {
  unsigned char hourPos = (now.hour()%12)*5;
  leds[(hourPos+LED_OFFSET)%60].r = 255;
  leds[(now.minute()+LED_OFFSET)%60].g = 255;
  leds[(now.second()+LED_OFFSET)%60].b = 255;
}

void basicClock(DateTime now) {
  unsigned char hourPos = (now.hour()%12)*5 + (now.minute()+6)/12;
  leds[(hourPos+LED_OFFSET+59)%60].r = 255;
  leds[(hourPos+LED_OFFSET+59)%60].g = 0;
  leds[(hourPos+LED_OFFSET+59)%60].b = 0;  
  leds[(hourPos+LED_OFFSET)%60].r = 255;
  leds[(hourPos+LED_OFFSET)%60].g = 0;
  leds[(hourPos+LED_OFFSET)%60].b = 0;
  leds[(hourPos+LED_OFFSET+1)%60].r = 255;
  leds[(hourPos+LED_OFFSET+1)%60].g = 0;
  leds[(hourPos+LED_OFFSET+1)%60].b = 0;
  leds[(now.minute()+LED_OFFSET)%60].r = 0;
  leds[(now.minute()+LED_OFFSET)%60].g = 255;
  leds[(now.minute()+LED_OFFSET)%60].b = 0;  
  leds[(now.second()+LED_OFFSET)%60].r = 0;
  leds[(now.second()+LED_OFFSET)%60].g = 0;
  leds[(now.second()+LED_OFFSET)%60].b = 255;
  
}

void smoothSecond(DateTime now) {
  if (now.second()!=old.second()) {
    old = now;
    cyclesPerSec = millis() - newSecTime;
    cyclesPerSecFloat = (float) cyclesPerSec;
    newSecTime = millis();      
  } 
  // set hour, min & sec LEDs
  fracOfSec = (millis() - newSecTime)/cyclesPerSecFloat;  // This divides by 733, but should be 1000 and not sure why???
  if (subSeconds < cyclesPerSec) {secondBrightness = 50.0*(1.0+sin((3.14*fracOfSec)-1.57));}
  if (subSeconds < cyclesPerSec) {secondBrightness2 = 50.0*(1.0+sin((3.14*fracOfSec)+1.57));}
  unsigned char hourPos = ((now.hour()%12)*5 + (now.minute()+6)/12);
  // The colours are set last, so if on same LED mixed colours are created
  leds[(hourPos+LED_OFFSET+59)%60].r = 255;   
  leds[(hourPos+LED_OFFSET)%60].r = 255;
  leds[(hourPos+LED_OFFSET+1)%60].r = 255;
  leds[(now.minute()+LED_OFFSET)%60].g = 255;
  leds[(now.second()+LED_OFFSET)%60].b = secondBrightness;
  leds[(now.second()+LED_OFFSET+59)%60].b = secondBrightness2;
}

void outlineClock(DateTime now) {
  for (int i = 0; i < NUM_LEDS; i += 5) {
    // Apply to every 5th LED (5-minute ticks)
    leds[i].r = 100;
    leds[i].g = 100;
    leds[i].b = 100;
  }
  unsigned char hourPos = ((now.hour()%12)*5 + (now.minute()+6)/12);
  leds[(hourPos+LED_OFFSET+59)%60].r = 255;   
  leds[(hourPos+LED_OFFSET)%60].r = 255;
  leds[(hourPos+LED_OFFSET+1)%60].r = 255;
  leds[(now.minute()+LED_OFFSET)%60].g = 255;
  leds[(now.second()+LED_OFFSET)%60].b = 255;
}

void minimalMilliSec(DateTime now) {
  if (now.second()!=old.second()) {
    old = now;
    cyclesPerSec = (millis() - newSecTime);
    newSecTime = millis();
  } 
  // set hour, min & sec LEDs
  unsigned char hourPos = ((now.hour()%12)*5 + (now.minute()+6)/12);
  subSeconds = (((millis() - newSecTime)*60)/cyclesPerSec)%60;  // This divides by 733, but should be 1000 and not sure why???
  // Millisec lights are set first, so hour/min/sec lights override and don't flicker as millisec passes
  leds[(subSeconds+LED_OFFSET)%60].r = 50;
  leds[(subSeconds+LED_OFFSET)%60].g = 50;
  leds[(subSeconds+LED_OFFSET)%60].b = 50;
  // The colours are set last, so if on same LED mixed colours are created
  leds[(hourPos+LED_OFFSET+59)%60].r = 255;   
  leds[(hourPos+LED_OFFSET)%60].r = 255;
  leds[(hourPos+LED_OFFSET+1)%60].r = 255;
  leds[(now.minute()+LED_OFFSET)%60].g = 255;
  leds[(now.second()+LED_OFFSET)%60].b = 255;
}

// Pendulum will be at the bottom and left for one second and right for one second
void simplePendulum(DateTime now) {
  if (now.second()!=old.second()) {
    old = now;
    cyclesPerSec = millis() - newSecTime;
    cyclesPerSecFloat = (float) cyclesPerSec;
    newSecTime = millis();
    if (swingBack == true) {swingBack = false;}
    else {swingBack = true;}
  } 
  // set hour, min & sec LEDs
  fracOfSec = (millis() - newSecTime)/cyclesPerSecFloat;  // This divides by 733, but should be 1000 and not sure why???
  if (subSeconds < cyclesPerSec && swingBack == true) {pendulumPos = 27.0 + 3.4*(1.0+sin((3.14*fracOfSec)-1.57));}
  if (subSeconds < cyclesPerSec && swingBack == false) {pendulumPos = 27.0 + 3.4*(1.0+sin((3.14*fracOfSec)+1.57));}
  unsigned char hourPos = ((now.hour()%12)*5 + (now.minute()+6)/12);
  // Pendulum lights are set first, so hour/min/sec lights override and don't flicker as millisec passes
  leds[(pendulumPos + LED_OFFSET)%60].r = 100;
  leds[(pendulumPos + LED_OFFSET)%60].g = 100;
  leds[(pendulumPos + LED_OFFSET)%60].b = 100;
  // The colours are set last, so if on same LED mixed colours are created
  leds[(hourPos+LED_OFFSET+59)%60].r = 255;   
  leds[(hourPos+LED_OFFSET)%60].r = 255;
  leds[(hourPos+LED_OFFSET+1)%60].r = 255;
  leds[(now.minute()+LED_OFFSET)%60].g = 255;
  leds[(now.second()+LED_OFFSET)%60].b = 255;
}

void breathingClock(DateTime now) {
  if (alarmTrig == false) {
    breathBrightness = 30.0*(1.0+sin((3.14*millis()/2000.0)-1.57)) + 2;
    for (int i = 0; i < NUM_LEDS; i += 5) {
      // Apply to every 5th LED (5-minute ticks)
      leds[i].r = breathBrightness;
      leds[i].g = breathBrightness;
      leds[i].b = breathBrightness;
    }
  }
  unsigned char hourPos = (now.hour()%12)*5 + (now.minute()+6)/12;
  leds[(hourPos+LED_OFFSET+59)%60].r = 255;   
  leds[(hourPos+LED_OFFSET)%60].r = 255;
  leds[(hourPos+LED_OFFSET+1)%60].r = 255;
  leds[(now.minute()+LED_OFFSET)%60].g = 255;
  leds[(now.second()+LED_OFFSET)%60].b = 255;
}


// // Cycle through the color wheel, equally spaced around the belt
// void rainbowCycle(uint8_t wait)
// {
//   uint16_t i, j1;
//   for (j1=0; j1 < 384 * 5; j1++) {     // 5 cycles of all 384 colors in the wheel
//     for (i=0; i < NUM_LEDS; i++)  {
//       // tricky math! we use each pixel as a fraction of the full 384-color
//       // wheel (thats the i / strip.numPixels() part)
//       // Then add in j which makes the colors go around per pixel
//       // the % 384 is to make the wheel cycle around
//       uint8_t colors[3];
//       wheel(((i * 384 / NUM_LEDS) + j) % 384, colors);
//       leds[i+LED_OFFSET].r = colors[0];
//       leds[i+LED_OFFSET].g = colors[1];
//       leds[i+LED_OFFSET].b = colors[2];
//     }
//     delay(wait);
//   }
// }

// //Input a value 0 to 384 to get a color value.
// //The colours are a transition r - g - b - back to r
// void wheel(uint16_t WheelPos, uint8_t colors[]) {
//   switch(WheelPos / 128)
//   {
//     case 0:
//       colors[0] = 127 - WheelPos % 128; // red down
//       colors[1] = WheelPos % 128;       // green up
//       colors[2] = 0;                    // blue off
//       break;
//     case 1:
//       colors[1] = 127 - WheelPos % 128; // green down
//       colors[2] = WheelPos % 128;       // blue up
//       colors[0] = 0;                    // red off
//       break;
//     case 2:
//       colors[2] = 127 - WheelPos % 128; // blue down
//       colors[0] = WheelPos % 128;       // red up
//       colors[1] = 0;                    // green off
//       break;
//   }
// }

