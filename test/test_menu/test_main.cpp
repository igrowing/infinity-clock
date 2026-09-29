#include <unity.h>
#include "menu_machine.h"

void setUp() {}
void tearDown() {}

static const uint8_t FACES = 8;

static NowInfo at(uint8_t h, uint8_t m, uint8_t s) {
  NowInfo n = {};
  n.year = 2026; n.month = 9; n.day = 29; n.hour = h; n.minute = m; n.second = s; n.unixtime = 1000000;
  return n;
}

// One loop iteration in which nothing but `rotary` happened.
static MenuEffects rotate(MenuModel& m, int rotary) {
  MenuInput in = {}; in.clockModeCount = FACES; in.now = at(10, 20, 30);
  m.rotaryMove = rotary;
  return menuStep(m, in);
}

// A press of `pressedMs` followed by the release, as two loop iterations. Returns the effects of the release.
static MenuEffects click(MenuModel& m, long pressedMs) {
  MenuInput in = {}; in.clockModeCount = FACES; in.now = at(10, 20, 30);
  in.pressed = true; in.heldMs = pressedMs;
  menuStep(m, in);
  in.pressed = false; in.released = true;
  return menuStep(m, in);
}

// A release without any press before it.
static MenuEffects releaseOnly(MenuModel& m) {
  MenuInput in = {}; in.clockModeCount = FACES; in.released = true; in.now = at(10, 20, 30);
  return menuStep(m, in);
}

static MenuModel in_state(State s) { MenuModel m; m.state = s; return m; }

// ================= CLOCK =================
void test_clock_rotate_changes_mode_and_saves() {
  MenuModel m = in_state(STATE_CLOCK); m.clockMode = 2;
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_EQUAL(3, m.clockMode); TEST_ASSERT_TRUE(fx.saveClockMode); TEST_ASSERT_EQUAL(0, m.rotaryMove);
}
void test_clock_mode_wraps_at_both_ends() {
  MenuModel m = in_state(STATE_CLOCK); m.clockMode = FACES - 1;
  rotate(m, +1); TEST_ASSERT_EQUAL(0, m.clockMode);
  rotate(m, -1); TEST_ASSERT_EQUAL(FACES - 1, m.clockMode);
}
void test_clock_short_click_opens_alarm_screen() {
  MenuModel m = in_state(STATE_CLOCK);
  MenuEffects fx = click(m, 200);
  TEST_ASSERT_EQUAL(STATE_ALARM, m.state); TEST_ASSERT_TRUE(fx.resetSecTimer);
}
void test_clock_long_click_opens_clock_setting() {
  MenuModel m = in_state(STATE_CLOCK);
  click(m, HOLD_TIME_MS + 100);
  TEST_ASSERT_EQUAL(STATE_SET_CLOCK_HR, m.state);
}
// The boot bug at machine level: a release with no press before it must be ignored.
void test_clock_release_without_press_is_ignored() {
  MenuModel m = in_state(STATE_CLOCK);
  releaseOnly(m);
  TEST_ASSERT_EQUAL(STATE_CLOCK, m.state);
}
void test_clock_rotate_wins_over_release() {
  MenuModel m = in_state(STATE_CLOCK);
  MenuInput in = {}; in.clockModeCount = FACES; in.released = true; in.now = at(1, 2, 3);
  m.rotaryMove = 1; m.menuPressSeen = true;
  menuStep(m, in);
  TEST_ASSERT_EQUAL(STATE_CLOCK, m.state); TEST_ASSERT_EQUAL(1, m.clockMode);
}

// ================= ALARM screen =================
void test_alarm_rotate_selects_mode_and_arms() {
  MenuModel m = in_state(STATE_ALARM);
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_EQUAL(1, m.alarmMode); TEST_ASSERT_EQUAL(1, m.alarmSet);
  TEST_ASSERT_TRUE(fx.saveAlarmMode); TEST_ASSERT_TRUE(fx.saveAlarmSet);
}
void test_alarm_mode_zero_disarms() {
  MenuModel m = in_state(STATE_ALARM); m.alarmMode = 1; m.alarmSet = 1;
  rotate(m, -1);
  TEST_ASSERT_EQUAL(0, m.alarmMode); TEST_ASSERT_EQUAL(0, m.alarmSet);
}
void test_alarm_mode_wraps() {
  MenuModel m = in_state(STATE_ALARM);
  rotate(m, -1); TEST_ASSERT_EQUAL(ALARM_MODE_MAX, m.alarmMode); TEST_ASSERT_EQUAL(1, m.alarmSet);
  rotate(m, +1); TEST_ASSERT_EQUAL(0, m.alarmMode); TEST_ASSERT_EQUAL(0, m.alarmSet);
}
void test_alarm_short_click_opens_timer() {
  MenuModel m = in_state(STATE_ALARM);
  MenuEffects fx = click(m, 200);
  TEST_ASSERT_EQUAL(STATE_COUNTDOWN, m.state); TEST_ASSERT_TRUE(fx.resetJ);
}
void test_alarm_long_click_opens_alarm_hour_setting() {
  MenuModel m = in_state(STATE_ALARM);
  click(m, HOLD_TIME_MS + 100);
  TEST_ASSERT_EQUAL(STATE_SET_ALARM_HR, m.state);
}

// ================= alarm time setting =================
void test_alarm_hour_rotate_and_wrap() {
  MenuModel m = in_state(STATE_SET_ALARM_HR); m.alarmHour = 23;
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_EQUAL(0, m.alarmHour); TEST_ASSERT_TRUE(fx.saveAlarmHour);
  rotate(m, -1); TEST_ASSERT_EQUAL(23, m.alarmHour);
}
void test_alarm_hour_click_goes_to_minutes() {
  MenuModel m = in_state(STATE_SET_ALARM_HR); m.alarmHour = 7;
  click(m, 200);
  TEST_ASSERT_EQUAL(STATE_SET_ALARM_MIN, m.state); TEST_ASSERT_EQUAL(7, m.alarmHour);
}
void test_alarm_minute_rotate_and_wrap() {
  MenuModel m = in_state(STATE_SET_ALARM_MIN); m.alarmMin = 59;
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_EQUAL(0, m.alarmMin); TEST_ASSERT_TRUE(fx.saveAlarmMin);
  rotate(m, -1); TEST_ASSERT_EQUAL(59, m.alarmMin);
}
void test_alarm_minute_click_returns_to_alarm_screen_and_rearms_today() {
  MenuModel m = in_state(STATE_SET_ALARM_MIN); m.alarmDay = 29;
  MenuEffects fx = click(m, 200);
  TEST_ASSERT_EQUAL(STATE_ALARM, m.state); TEST_ASSERT_EQUAL(0, m.alarmDay); TEST_ASSERT_TRUE(fx.resetSecTimer);
}

// ================= clock time setting =================
void test_set_hour_rotates_only_the_hour() {
  MenuModel m = in_state(STATE_SET_CLOCK_HR);
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_TRUE(fx.setTime);
  TEST_ASSERT_EQUAL(11, fx.newTime.hour); TEST_ASSERT_EQUAL(20, fx.newTime.minute); TEST_ASSERT_EQUAL(30, fx.newTime.second);
  TEST_ASSERT_EQUAL(2026, fx.newTime.year); TEST_ASSERT_EQUAL(29, fx.newTime.day);
}
void test_set_hour_wraps() {
  MenuModel m = in_state(STATE_SET_CLOCK_HR);
  MenuInput in = {}; in.clockModeCount = FACES; in.now = at(0, 5, 5); m.rotaryMove = -1;
  TEST_ASSERT_EQUAL(23, menuStep(m, in).newTime.hour);
}
void test_set_minute_rotates_only_the_minute() {
  MenuModel m = in_state(STATE_SET_CLOCK_MIN);
  MenuEffects fx = rotate(m, -1);
  TEST_ASSERT_EQUAL(10, fx.newTime.hour); TEST_ASSERT_EQUAL(19, fx.newTime.minute); TEST_ASSERT_EQUAL(30, fx.newTime.second);
}
void test_set_second_rotates_only_the_second() {
  MenuModel m = in_state(STATE_SET_CLOCK_SEC);
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_EQUAL(10, fx.newTime.hour); TEST_ASSERT_EQUAL(20, fx.newTime.minute); TEST_ASSERT_EQUAL(31, fx.newTime.second);
}
void test_set_clock_clicks_walk_through_the_fields() {
  MenuModel m = in_state(STATE_SET_CLOCK_HR);
  click(m, 200); TEST_ASSERT_EQUAL(STATE_SET_CLOCK_MIN, m.state);
  click(m, 200); TEST_ASSERT_EQUAL(STATE_SET_CLOCK_SEC, m.state);
  click(m, 200); TEST_ASSERT_EQUAL(STATE_SET_CLOCK_UP, m.state);
  click(m, 200); TEST_ASSERT_EQUAL(STATE_CLOCK, m.state);
}
void test_set_clock_asks_to_print_the_time() {
  MenuModel m = in_state(STATE_SET_CLOCK_HR);
  TEST_ASSERT_TRUE(rotate(m, +1).printDateTime);
  m = in_state(STATE_SET_CLOCK_UP);
  TEST_ASSERT_FALSE(rotate(m, +1).printDateTime);
}
void test_led_offset_toggles_both_ways_and_saves() {
  MenuModel m = in_state(STATE_SET_CLOCK_UP); m.ledOffset = LED_OFFSET_L;
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_EQUAL(LED_OFFSET_R, m.ledOffset); TEST_ASSERT_TRUE(fx.saveLedOffset);
  rotate(m, +1); TEST_ASSERT_EQUAL(LED_OFFSET_L, m.ledOffset);
}

// ================= timer =================
void test_timer_rotate_sets_minutes_silently() {
  MenuModel m = in_state(STATE_COUNTDOWN);
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_EQUAL(60, m.countDownTime); TEST_ASSERT_FALSE(m.countDown); TEST_ASSERT_TRUE(fx.stopBuzzer);
  TEST_ASSERT_EQUAL(STATE_COUNTDOWN, m.state);
}
void test_timer_short_click_starts_countdown() {
  MenuModel m = in_state(STATE_COUNTDOWN); m.countDownTime = 300;
  click(m, 200);
  TEST_ASSERT_TRUE(m.countDown); TEST_ASSERT_EQUAL(1000000, m.startCountDown); TEST_ASSERT_EQUAL(STATE_COUNTDOWN, m.state);
}
void test_timer_short_click_without_time_starts_demo() {
  MenuModel m = in_state(STATE_COUNTDOWN);
  MenuEffects fx = click(m, 200);
  TEST_ASSERT_EQUAL(STATE_DEMO, m.state); TEST_ASSERT_TRUE(fx.startDemo); TEST_ASSERT_TRUE(fx.resetJ);
}
void test_timer_long_click_resets_timer() {
  MenuModel m = in_state(STATE_COUNTDOWN); m.countDown = true; m.countDownTime = 300; m.currentCountDown = 120;
  MenuEffects fx = click(m, HOLD_TIME_MS + 100);
  TEST_ASSERT_FALSE(m.countDown); TEST_ASSERT_EQUAL(0, m.countDownTime); TEST_ASSERT_EQUAL(0, m.currentCountDown);
  TEST_ASSERT_TRUE(fx.resetJ); TEST_ASSERT_EQUAL(STATE_COUNTDOWN, m.state);
}
void test_demo_click_returns_to_clock() {
  MenuModel m = in_state(STATE_DEMO);
  click(m, 200);
  TEST_ASSERT_EQUAL(STATE_CLOCK, m.state);
}

// ================= stop rules: timer =================
void test_running_timer_stops_on_short_click() {
  MenuModel m = in_state(STATE_COUNTDOWN); m.countDown = true; m.countDownTime = 300; m.currentCountDown = 120;
  MenuEffects fx = click(m, 200);
  TEST_ASSERT_FALSE(m.countDown); TEST_ASSERT_EQUAL(0, m.countDownTime); TEST_ASSERT_EQUAL(0, m.currentCountDown);
  TEST_ASSERT_TRUE(fx.clearBuzzer); TEST_ASSERT_EQUAL(STATE_CLOCK, m.state);
}
void test_finished_timer_stops_on_short_click() {
  MenuModel m = in_state(STATE_COUNTDOWN); m.countDown = true; m.countDownTime = 300; m.currentCountDown = 0;
  MenuEffects fx = click(m, 200);
  TEST_ASSERT_TRUE(fx.clearBuzzer); TEST_ASSERT_EQUAL(STATE_CLOCK, m.state); TEST_ASSERT_FALSE(m.countDown);
}
void test_finished_timer_stops_on_rotate() {
  MenuModel m = in_state(STATE_COUNTDOWN); m.countDown = true; m.countDownTime = 300; m.currentCountDown = 0;
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_TRUE(fx.clearBuzzer); TEST_ASSERT_EQUAL(STATE_CLOCK, m.state);
  TEST_ASSERT_FALSE(m.countDown); TEST_ASSERT_EQUAL(0, m.countDownTime);
}
void test_rotate_on_running_timer_changes_time_and_stops_it() {
  MenuModel m = in_state(STATE_COUNTDOWN); m.countDown = true; m.countDownTime = 600; m.currentCountDown = 300;
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_FALSE(m.countDown); TEST_ASSERT_EQUAL(360, m.countDownTime); TEST_ASSERT_TRUE(fx.stopBuzzer);
}

// ================= stop rules: alarm =================
static MenuModel ringing(State s) { MenuModel m = in_state(s); m.alarmTrig = true; m.clockMode = 4; return m; }

void test_ringing_alarm_stops_on_click() {
  MenuModel m = ringing(STATE_CLOCK);
  MenuInput in = {}; in.clockModeCount = FACES; in.now = at(7, 0, 0); in.pressed = true; in.heldMs = 10;
  MenuEffects fx = menuStep(m, in);
  TEST_ASSERT_FALSE(m.alarmTrig); TEST_ASSERT_TRUE(fx.clearBuzzer); TEST_ASSERT_TRUE(fx.waitAfterAlarmCancel);
  TEST_ASSERT_EQUAL(STATE_CLOCK, m.state); TEST_ASSERT_EQUAL(29, m.alarmDay);
}
void test_ringing_alarm_stops_on_rotate() {
  MenuModel m = ringing(STATE_CLOCK);
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_FALSE(m.alarmTrig); TEST_ASSERT_TRUE(fx.clearBuzzer); TEST_ASSERT_EQUAL(29, m.alarmDay);
}
void test_ringing_alarm_stops_from_any_screen() {
  MenuModel m = ringing(STATE_COUNTDOWN); m.countDown = true;
  MenuEffects fx = rotate(m, +1);
  TEST_ASSERT_EQUAL(STATE_CLOCK, m.state); TEST_ASSERT_FALSE(m.countDown); TEST_ASSERT_TRUE(fx.clearBuzzer);
}
// The code comment says stopping the alarm performs "no actions", so neither the click nor the rotation that
// stopped it may leak into the clock afterwards.
// BUG: the release that ends the stopping click is handled as a normal short click and opens the alarm screen.
void test_click_that_stops_alarm_stays_on_clock() {
  MenuModel m = ringing(STATE_CLOCK);
  click(m, 200);
  TEST_ASSERT_FALSE(m.alarmTrig);
  TEST_ASSERT_EQUAL(STATE_CLOCK, m.state);
}
// BUG: the rotation that stopped the alarm is not consumed, so the next loop iteration also changes the clock mode.
void test_rotation_that_stops_alarm_is_consumed() {
  MenuModel m = ringing(STATE_CLOCK);
  rotate(m, +1);
  TEST_ASSERT_EQUAL(0, m.rotaryMove);
  TEST_ASSERT_EQUAL(4, m.clockMode);
}

// Press events only, no release.
static void hold(MenuModel& m, long heldMs) {
  MenuInput in = {}; in.clockModeCount = FACES; in.now = at(7, 0, 0); in.pressed = true; in.heldMs = heldMs;
  menuStep(m, in);
}
void test_slow_click_that_stops_alarm_stays_on_clock() {
  MenuModel m = ringing(STATE_CLOCK);
  hold(m, 10);       // stops the alarm
  hold(m, 500);      // still held after the 300 ms wait
  hold(m, HOLD_TIME_MS + 500);  // a long hold must not open the clock setting either
  MenuInput in = {}; in.clockModeCount = FACES; in.now = at(7, 0, 0); in.released = true;
  menuStep(m, in);
  TEST_ASSERT_EQUAL(STATE_CLOCK, m.state); TEST_ASSERT_FALSE(m.countTime); TEST_ASSERT_FALSE(m.menuPressSeen);
}
void test_click_after_the_one_that_stopped_the_alarm_works_normally() {
  MenuModel m = ringing(STATE_CLOCK);
  click(m, 200);
  click(m, 200);
  TEST_ASSERT_EQUAL(STATE_ALARM, m.state);
}
void test_rotation_that_stops_alarm_does_not_swallow_the_next_click() {
  MenuModel m = ringing(STATE_CLOCK);
  rotate(m, +1);
  click(m, 200);
  TEST_ASSERT_EQUAL(STATE_ALARM, m.state);
}
void test_rotation_while_the_stopping_click_is_held_is_ignored() {
  MenuModel m = ringing(STATE_CLOCK);
  hold(m, 10);
  rotate(m, +1);
  TEST_ASSERT_EQUAL(4, m.clockMode); TEST_ASSERT_EQUAL(0, m.rotaryMove);
}
void test_alarm_stopped_by_release_forgets_the_press() {
  MenuModel m = ringing(STATE_CLOCK); m.menuPressSeen = true;
  releaseOnly(m);
  TEST_ASSERT_FALSE(m.menuPressSeen); TEST_ASSERT_FALSE(m.alarmTrig); TEST_ASSERT_EQUAL(STATE_CLOCK, m.state);
}

// ================= button bookkeeping =================
void test_press_is_remembered_and_release_forgets_it() {
  MenuModel m = in_state(STATE_CLOCK);
  MenuInput in = {}; in.clockModeCount = FACES; in.now = at(1, 1, 1); in.pressed = true; in.heldMs = 250;
  menuStep(m, in);
  TEST_ASSERT_TRUE(m.countTime); TEST_ASSERT_TRUE(m.menuPressSeen); TEST_ASSERT_EQUAL(250, m.menuTimePressed);
  in.pressed = false; in.released = true;
  menuStep(m, in);
  TEST_ASSERT_FALSE(m.countTime); TEST_ASSERT_FALSE(m.menuPressSeen);
}
void test_display_blinks_only_just_before_the_hold_limit() {
  MenuModel m = in_state(STATE_CLOCK);
  MenuInput in = {}; in.clockModeCount = FACES; in.now = at(1, 1, 1); in.pressed = true;
  in.heldMs = HOLD_TIME_MS - 101; TEST_ASSERT_FALSE(menuStep(m, in).blinkDisplay);
  in.heldMs = HOLD_TIME_MS - 100; TEST_ASSERT_TRUE(menuStep(m, in).blinkDisplay);
  in.heldMs = HOLD_TIME_MS;       TEST_ASSERT_TRUE(menuStep(m, in).blinkDisplay);
  in.heldMs = HOLD_TIME_MS + 1;   TEST_ASSERT_FALSE(menuStep(m, in).blinkDisplay);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_clock_rotate_changes_mode_and_saves);
  RUN_TEST(test_clock_mode_wraps_at_both_ends);
  RUN_TEST(test_clock_short_click_opens_alarm_screen);
  RUN_TEST(test_clock_long_click_opens_clock_setting);
  RUN_TEST(test_clock_release_without_press_is_ignored);
  RUN_TEST(test_clock_rotate_wins_over_release);
  RUN_TEST(test_alarm_rotate_selects_mode_and_arms);
  RUN_TEST(test_alarm_mode_zero_disarms);
  RUN_TEST(test_alarm_mode_wraps);
  RUN_TEST(test_alarm_short_click_opens_timer);
  RUN_TEST(test_alarm_long_click_opens_alarm_hour_setting);
  RUN_TEST(test_alarm_hour_rotate_and_wrap);
  RUN_TEST(test_alarm_hour_click_goes_to_minutes);
  RUN_TEST(test_alarm_minute_rotate_and_wrap);
  RUN_TEST(test_alarm_minute_click_returns_to_alarm_screen_and_rearms_today);
  RUN_TEST(test_set_hour_rotates_only_the_hour);
  RUN_TEST(test_set_hour_wraps);
  RUN_TEST(test_set_minute_rotates_only_the_minute);
  RUN_TEST(test_set_second_rotates_only_the_second);
  RUN_TEST(test_set_clock_clicks_walk_through_the_fields);
  RUN_TEST(test_set_clock_asks_to_print_the_time);
  RUN_TEST(test_led_offset_toggles_both_ways_and_saves);
  RUN_TEST(test_timer_rotate_sets_minutes_silently);
  RUN_TEST(test_timer_short_click_starts_countdown);
  RUN_TEST(test_timer_short_click_without_time_starts_demo);
  RUN_TEST(test_timer_long_click_resets_timer);
  RUN_TEST(test_demo_click_returns_to_clock);
  RUN_TEST(test_running_timer_stops_on_short_click);
  RUN_TEST(test_finished_timer_stops_on_short_click);
  RUN_TEST(test_finished_timer_stops_on_rotate);
  RUN_TEST(test_rotate_on_running_timer_changes_time_and_stops_it);
  RUN_TEST(test_ringing_alarm_stops_on_click);
  RUN_TEST(test_ringing_alarm_stops_on_rotate);
  RUN_TEST(test_ringing_alarm_stops_from_any_screen);
  RUN_TEST(test_click_that_stops_alarm_stays_on_clock);
  RUN_TEST(test_rotation_that_stops_alarm_is_consumed);
  RUN_TEST(test_slow_click_that_stops_alarm_stays_on_clock);
  RUN_TEST(test_click_after_the_one_that_stopped_the_alarm_works_normally);
  RUN_TEST(test_rotation_that_stops_alarm_does_not_swallow_the_next_click);
  RUN_TEST(test_rotation_while_the_stopping_click_is_held_is_ignored);
  RUN_TEST(test_alarm_stopped_by_release_forgets_the_press);
  RUN_TEST(test_press_is_remembered_and_release_forgets_it);
  RUN_TEST(test_display_blinks_only_just_before_the_hold_limit);
  return UNITY_END();
}
