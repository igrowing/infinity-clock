#include "menu_machine.h"

// Silence the buzzer, leave any ringing state and go back to the clock.
static void returnToClock(MenuModel& m, MenuEffects& fx) {
  fx.clearBuzzer = true;
  m.state = STATE_CLOCK;
  m.countDown = false;
}

static void resetTimer(MenuModel& m) {
  m.countDown = false;
  m.countDownTime = 0;
  m.currentCountDown = 0;
}

static void requestTime(const NowInfo& now, MenuEffects& fx, uint8_t hour, uint8_t minute, uint8_t second) {
  fx.setTime = true;
  fx.newTime = now;
  fx.newTime.hour = hour;
  fx.newTime.minute = minute;
  fx.newTime.second = second;
}

static void stepClock(MenuModel& m, const MenuInput& in, MenuEffects& fx) {
  if (m.rotaryMove != 0) {  // Progress next mode from current mode.
    m.clockMode = wrapStep(m.clockMode, m.rotaryMove, in.clockModeCount);
    fx.saveClockMode = true;
    m.rotaryMove = 0;
  } else if (in.released) {
    m.state = stateOnMenuRelease(m.menuPressSeen, m.menuTimePressed);
    if (m.state == STATE_ALARM) fx.resetSecTimer = true;
  }
}

static void stepAlarm(MenuModel& m, const MenuInput& in, MenuEffects& fx) {
  if (m.rotaryMove != 0) {
    m.alarmMode = wrapStep(m.alarmMode, m.rotaryMove, ALARM_MODE_MAX + 1);  // Mode 0 is alarm off
    m.alarmSet = (m.alarmMode == 0) ? 0 : 1;
  }
  fx.saveAlarmSet = true;
  fx.saveAlarmMode = true;
  m.rotaryMove = 0;
  m.alarmTrig = false;
  if (in.released) {
    if (m.menuTimePressed <= HOLD_TIME_MS) {m.state = STATE_COUNTDOWN; fx.resetSweep = true;}
    else m.state = STATE_SET_ALARM_HR;
  }
}

static void stepSetAlarmHour(MenuModel& m, const MenuInput& in, MenuEffects& fx) {
  if (in.released) m.state = STATE_SET_ALARM_MIN;
  else m.alarmHour = wrapStep(m.alarmHour, m.rotaryMove, 24);
  fx.saveAlarmHour = true;
  m.rotaryMove = 0;
}

static void stepSetAlarmMinute(MenuModel& m, const MenuInput& in, MenuEffects& fx) {
  if (in.released) {
    m.state = STATE_ALARM;
    m.alarmDay = 0;  // A newly set alarm may ring today
    fx.resetSecTimer = true;
  } else {
    m.alarmMin = wrapStep(m.alarmMin, m.rotaryMove, 60);
  }
  fx.saveAlarmMin = true;
  m.rotaryMove = 0;
}

static void stepSetClock(MenuModel& m, const MenuInput& in, MenuEffects& fx) {
  const NowInfo& now = in.now;
  if (in.released) {
    m.state = (m.state == STATE_SET_CLOCK_HR) ? STATE_SET_CLOCK_MIN :
              (m.state == STATE_SET_CLOCK_MIN) ? STATE_SET_CLOCK_SEC : STATE_SET_CLOCK_UP;
  } else if (m.rotaryMove != 0) {
    if (m.state == STATE_SET_CLOCK_HR) requestTime(now, fx, wrapStep(now.hour, m.rotaryMove, 24), now.minute, now.second);
    else if (m.state == STATE_SET_CLOCK_MIN) requestTime(now, fx, now.hour, wrapStep(now.minute, m.rotaryMove, 60), now.second);
    else requestTime(now, fx, now.hour, now.minute, wrapStep(now.second, m.rotaryMove, 60));
    m.rotaryMove = 0;
  }
}

static void stepSetLedOffset(MenuModel& m, const MenuInput& in, MenuEffects& fx) {
  if (in.released) {
    m.state = STATE_CLOCK;
  } else if (m.rotaryMove != 0) {
    m.ledOffset = (m.ledOffset == LED_OFFSET_L) ? LED_OFFSET_R : LED_OFFSET_L;
    fx.saveLedOffset = true;
    m.rotaryMove = 0;
  }
}

static void stepCountdown(MenuModel& m, const MenuInput& in, MenuEffects& fx) {
  if (in.released) {
    if (m.menuTimePressed <= HOLD_TIME_MS) {
      if (m.countDown) {  // Timer on + quick click ==> stop countdown.
        resetTimer(m);
        returnToClock(m, fx);
      } else if (m.countDownTime > 0) {  // Timer off, there is time + quick click ==> start countdown.
        m.countDown = true;
        m.startCountDown = in.now.unixtime;
      } else {  // Timer off, no time + quick click ==> demo.
        m.state = STATE_DEMO;
        fx.startDemo = true;
        fx.resetSweep = true;
      }
    } else {  // Long click ==> reset the timer.
      resetTimer(m);
      fx.resetSweep = true;
    }
  } else if (m.rotaryMove != 0) {
    if (m.countDown && m.currentCountDown <= 0) {  // Time is gone + rotated ==> stop countdown and buzzer.
      resetTimer(m);
      returnToClock(m, fx);
    } else {  // Set the timer silently.
      m.countDown = false;
      fx.stopBuzzer = true;
      m.countDownTime = countdownAfterRotate(m.currentCountDown, m.rotaryMove);
    }
  }
  m.rotaryMove = 0;
}

MenuEffects menuStep(MenuModel& m, const MenuInput& in) {
  MenuEffects fx = {};

  m.countTime = in.pressed;
  if (in.pressed) {
    m.menuPressSeen = true;
    m.menuTimePressed = in.heldMs;
    if (m.menuTimePressed >= (HOLD_TIME_MS - 100) && m.menuTimePressed <= HOLD_TIME_MS) fx.blinkDisplay = true;  // long click
  }

  // The press that stopped the alarm is still going: ignore everything up to and including its release.
  if (m.ignoreUntilRelease) {
    m.rotaryMove = 0;
    if (in.released) {
      m.ignoreUntilRelease = false;
      m.countTime = false;
      m.menuPressSeen = false;
    }
    return fx;
  }

  // Stop the alarm on click or rotate, and do nothing else.
  if (m.alarmTrig) {
    m.alarmTrig = false;
    returnToClock(m, fx);
    m.alarmDay = in.now.day;  // Not ringing again until the next day.
    m.rotaryMove = 0;         // The rotation that stopped the alarm is used up.
    m.ignoreUntilRelease = in.pressed;  // A click that stopped the alarm must not act when it is released.
    if (in.released) m.menuPressSeen = false;
    fx.waitAfterAlarmCancel = true;
    return fx;
  }

  switch (m.state) {
    case STATE_CLOCK:          stepClock(m, in, fx); break;
    case STATE_ALARM:          stepAlarm(m, in, fx); break;
    case STATE_SET_ALARM_HR:   stepSetAlarmHour(m, in, fx); break;
    case STATE_SET_ALARM_MIN:  stepSetAlarmMinute(m, in, fx); break;
    case STATE_SET_CLOCK_HR:
    case STATE_SET_CLOCK_MIN:
    case STATE_SET_CLOCK_SEC:  stepSetClock(m, in, fx); break;
    case STATE_SET_CLOCK_UP:   stepSetLedOffset(m, in, fx); break;
    case STATE_COUNTDOWN:      stepCountdown(m, in, fx); break;
    case STATE_DEMO:           if (in.released) m.state = STATE_CLOCK; break;
  }

  if (m.state == STATE_SET_CLOCK_HR || m.state == STATE_SET_CLOCK_MIN || m.state == STATE_SET_CLOCK_SEC) fx.printDateTime = true;
  if (in.released || m.rotaryMove != 0) m.countTime = false;
  if (in.released) m.menuPressSeen = false;
  return fx;
}

AlarmPhase alarmPhase(const MenuModel& m, const NowInfo& now) {
  // alarmDay is the day the alarm was cancelled on: a cancelled alarm must not restart within its minute.
  if (!m.alarmSet || m.alarmDay == now.day) return ALARM_IDLE;
  if (m.alarmTrig) return ALARM_RINGING;
  // Don't alarm while the alarm time is being set.
  if (m.state == STATE_SET_ALARM_HR || m.state == STATE_SET_ALARM_MIN) return ALARM_IDLE;
  if (m.alarmMin == now.minute && m.alarmHour == now.hour) return ALARM_DUE;
  return ALARM_IDLE;
}
