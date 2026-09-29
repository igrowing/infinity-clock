#include <unity.h>
#include "clock_logic.h"

void setUp() {}
void tearDown() {}

// ---- wrapStep ----
void test_wrap_forward()          { TEST_ASSERT_EQUAL(6, wrapStep(5, 1, 60)); }
void test_wrap_forward_at_end()   { TEST_ASSERT_EQUAL(0, wrapStep(59, 1, 60)); }
void test_wrap_backward()         { TEST_ASSERT_EQUAL(4, wrapStep(5, -1, 60)); }
void test_wrap_backward_at_zero() { TEST_ASSERT_EQUAL(59, wrapStep(0, -1, 60)); }
void test_wrap_hours()            { TEST_ASSERT_EQUAL(23, wrapStep(0, -1, 24)); TEST_ASSERT_EQUAL(0, wrapStep(23, 1, 24)); }
void test_wrap_alarm_mode()       { TEST_ASSERT_EQUAL(3, wrapStep(0, -1, 4)); TEST_ASSERT_EQUAL(0, wrapStep(3, 1, 4)); }

// ---- countdownAfterRotate ----
void test_countdown_add_minute()      { TEST_ASSERT_EQUAL(120, countdownAfterRotate(60, 1)); }
void test_countdown_remove_minute()   { TEST_ASSERT_EQUAL(60, countdownAfterRotate(120, -1)); }
void test_countdown_left_from_zero()  { TEST_ASSERT_EQUAL(3600, countdownAfterRotate(0, -1)); }
void test_countdown_right_from_59_gives_60()  { TEST_ASSERT_EQUAL(3600, countdownAfterRotate(59 * 60, 1)); }
void test_countdown_right_from_60_wraps_to_0() { TEST_ASSERT_EQUAL(0, countdownAfterRotate(3600, 1)); }
void test_countdown_left_from_60_gives_59()   { TEST_ASSERT_EQUAL(59 * 60, countdownAfterRotate(3600, -1)); }
// Turning left from 0 gives 60 min, and turning right again must return to 0.
void test_countdown_left_then_right_returns_to_start() {
  long t = countdownAfterRotate(0, -1);
  TEST_ASSERT_EQUAL(0, countdownAfterRotate(t, 1));
}

// ---- alarmRampPosition ----
void test_ramp_start()        { TEST_ASSERT_EQUAL(0, alarmRampPosition(0)); }
void test_ramp_one_second()   { TEST_ASSERT_EQUAL(3, alarmRampPosition(1000)); }
void test_ramp_top_of_range() { TEST_ASSERT_EQUAL(29, alarmRampPosition(29 * 300)); }
// The position must not overflow (was int8_t: negative after ~38 s, restarting the ramp at ~77 s).
void test_ramp_after_40_seconds_is_not_negative() { TEST_ASSERT_TRUE(alarmRampPosition(40000) >= 0); }
void test_ramp_after_40_seconds_value()           { TEST_ASSERT_EQUAL(133, (int)alarmRampPosition(40000)); }
void test_ramp_after_2_minutes_keeps_growing()    { TEST_ASSERT_EQUAL(400, (int)alarmRampPosition(120000)); }
void test_ramp_saturates_instead_of_wrapping()    { TEST_ASSERT_EQUAL(32767, (int)alarmRampPosition(4000000000u)); }

// ---- stateOnMenuRelease ----
void test_release_after_short_click_opens_alarm() { TEST_ASSERT_EQUAL(STATE_ALARM, stateOnMenuRelease(true, 200)); }
void test_release_after_hold_limit_opens_alarm()  { TEST_ASSERT_EQUAL(STATE_ALARM, stateOnMenuRelease(true, HOLD_TIME_MS)); }
void test_release_after_long_click_opens_clock_setting() { TEST_ASSERT_EQUAL(STATE_SET_CLOCK_HR, stateOnMenuRelease(true, HOLD_TIME_MS + 1)); }
// BUG 2 (spurious alarm mode after boot). The global Bounce object is attached to the button pin BEFORE
// setup() enables INPUT_PULLUP, so a floating pin can be latched as LOW. When the pull-up kicks in,
// the first Bounce::update() reports a "rose" edge. No press ever happened (pressedMs is still its
// initial 0), yet the release was treated as a short click and the clock jumped to alarm mode.
void test_release_without_press_at_boot_stays_in_clock() {
  TEST_ASSERT_EQUAL(STATE_CLOCK, stateOnMenuRelease(false, 0));
}

// ---- sanitizeSettings ----
static const uint8_t FACES = 8;
static Settings valid() { return Settings{3, 30, 7, 1, 2, LED_OFFSET_R}; }
static bool same(Settings a, Settings b) {
  return a.clockMode == b.clockMode && a.alarmMin == b.alarmMin && a.alarmHour == b.alarmHour &&
         a.alarmSet == b.alarmSet && a.alarmMode == b.alarmMode && a.ledOffset == b.ledOffset;
}
void test_settings_valid_are_kept() { TEST_ASSERT_TRUE(same(valid(), sanitizeSettings(valid(), FACES))); }
void test_settings_virgin_ff_gets_defaults() {
  Settings r = sanitizeSettings(Settings{255, 255, 255, 255, 255, 255}, FACES);
  TEST_ASSERT_TRUE(same(Settings{0, 0, 0, 0, 0, LED_OFFSET_L}, r));
}
void test_settings_virgin_zero_gets_valid_led_offset() {
  Settings r = sanitizeSettings(Settings{0, 0, 0, 0, 0, 0}, FACES);
  TEST_ASSERT_EQUAL(LED_OFFSET_L, r.ledOffset);
}
void test_settings_clock_mode_bounds() {
  Settings s = valid();
  s.clockMode = FACES - 1; TEST_ASSERT_EQUAL(FACES - 1, sanitizeSettings(s, FACES).clockMode);
  s.clockMode = FACES;     TEST_ASSERT_EQUAL(0, sanitizeSettings(s, FACES).clockMode);
}
void test_settings_alarm_time_bounds() {
  Settings s = valid();
  s.alarmMin = 59;  s.alarmHour = 23; TEST_ASSERT_TRUE(same(s, sanitizeSettings(s, FACES)));
  s.alarmMin = 60;  s.alarmHour = 24; s = sanitizeSettings(s, FACES);
  TEST_ASSERT_EQUAL(0, s.alarmMin); TEST_ASSERT_EQUAL(0, s.alarmHour);
}
void test_settings_alarm_set_flag() {
  Settings s = valid();
  s.alarmSet = 0; TEST_ASSERT_EQUAL(0, sanitizeSettings(s, FACES).alarmSet);
  s.alarmSet = 1; TEST_ASSERT_EQUAL(1, sanitizeSettings(s, FACES).alarmSet);
  s.alarmSet = 2; TEST_ASSERT_EQUAL(0, sanitizeSettings(s, FACES).alarmSet);
}
void test_settings_every_alarm_mode_survives_reboot() {
  for (uint8_t m = 0; m <= ALARM_MODE_MAX; m++) {
    Settings s = valid(); s.alarmMode = m;
    TEST_ASSERT_EQUAL(m, sanitizeSettings(s, FACES).alarmMode);
  }
}
void test_settings_alarm_mode_out_of_range() {
  Settings s = valid(); s.alarmMode = ALARM_MODE_MAX + 1;
  TEST_ASSERT_EQUAL(0, sanitizeSettings(s, FACES).alarmMode);
}
void test_settings_led_offset_both_valid() {
  Settings s = valid();
  s.ledOffset = LED_OFFSET_L; TEST_ASSERT_EQUAL(LED_OFFSET_L, sanitizeSettings(s, FACES).ledOffset);
  s.ledOffset = LED_OFFSET_R; TEST_ASSERT_EQUAL(LED_OFFSET_R, sanitizeSettings(s, FACES).ledOffset);
  s.ledOffset = 10;           TEST_ASSERT_EQUAL(LED_OFFSET_L, sanitizeSettings(s, FACES).ledOffset);
}

// ---- alarmFadeBrightness ----
void test_fade_starts_dark()          { TEST_ASSERT_EQUAL(0, alarmFadeBrightness(0)); }
void test_fade_full_at_the_end()      { TEST_ASSERT_EQUAL(255, alarmFadeBrightness(FADE_TIME_MS)); }
void test_fade_full_after_the_end()   { TEST_ASSERT_EQUAL(255, alarmFadeBrightness(FADE_TIME_MS + 1)); }
// BUG: the elapsed/FADE_TIME_MS division is done in integers, so the fade stays at 0 for the whole first
// minute and then jumps to full brightness.
void test_fade_is_already_rising_at_10_seconds() { TEST_ASSERT_TRUE(alarmFadeBrightness(10000) > 0); }
void test_fade_halfway_value()        { TEST_ASSERT_INT_WITHIN(1, 74, alarmFadeBrightness(FADE_TIME_MS / 2)); }
void test_fade_keeps_rising()         { TEST_ASSERT_TRUE(alarmFadeBrightness(45000) > alarmFadeBrightness(30000)); }

// ---- pendulumLed ----
static const float HALF_PI_T = 1.5707963f;
static const int OFFSETS[2] = {LED_OFFSET_L, LED_OFFSET_R};

// Position on the clock face (0 = 12 o'clock, 30 = bottom) relative to the bottom: -3 = three LEDs to the left.
static int fromBottom(int stripIndex, int ledOffset) {
  int p = ((stripIndex - ledOffset) % NUM_LEDS + NUM_LEDS) % NUM_LEDS;
  return ((p - NUM_LEDS/2 + 90) % NUM_LEDS) - 30;
}
void test_pendulum_is_at_the_bottom_in_the_middle_of_a_swing() {
  for (int i = 0; i < 2; i++) {
    TEST_ASSERT_EQUAL(0, fromBottom(pendulumLed(0.5f, +HALF_PI_T, OFFSETS[i]), OFFSETS[i]));
    TEST_ASSERT_EQUAL(0, fromBottom(pendulumLed(0.5f, -HALF_PI_T, OFFSETS[i]), OFFSETS[i]));
  }
}
void test_pendulum_swings_equally_to_both_sides() {
  for (int i = 0; i < 2; i++) {
    for (int sw = 0; sw < 2; sw++) {
      float swing = sw ? -HALF_PI_T : HALF_PI_T;
      int lo = 99, hi = -99;
      for (int k = 0; k <= 1000; k++) {
        int d = fromBottom(pendulumLed(k / 1000.0f, swing, OFFSETS[i]), OFFSETS[i]);
        if (d < lo) lo = d;
        if (d > hi) hi = d;
      }
      TEST_ASSERT_EQUAL(-3, lo);
      TEST_ASSERT_EQUAL(3, hi);
    }
  }
}
void test_pendulum_moves_clockwise_first_then_back() {
  for (int i = 0; i < 2; i++) {
    TEST_ASSERT_EQUAL(-3, fromBottom(pendulumLed(0.0f, +HALF_PI_T, OFFSETS[i]), OFFSETS[i]));
    TEST_ASSERT_EQUAL(+3, fromBottom(pendulumLed(1.0f, +HALF_PI_T, OFFSETS[i]), OFFSETS[i]));
    TEST_ASSERT_EQUAL(+3, fromBottom(pendulumLed(0.0f, -HALF_PI_T, OFFSETS[i]), OFFSETS[i]));
    TEST_ASSERT_EQUAL(-3, fromBottom(pendulumLed(1.0f, -HALF_PI_T, OFFSETS[i]), OFFSETS[i]));
  }
}
void test_pendulum_is_always_a_valid_led() {
  float bad[] = {-5.0f, 0.0f, 0.5f, 1.0f, 3.0f, 1.0e9f, 0.0f / 0.0f, 1.0f / 0.0f, -1.0f / 0.0f};
  for (int i = 0; i < 2; i++) {
    for (unsigned b = 0; b < sizeof(bad)/sizeof(bad[0]); b++) {
      int led = pendulumLed(bad[b], HALF_PI_T, OFFSETS[i]);
      TEST_ASSERT_TRUE(led >= 0 && led < NUM_LEDS);
    }
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_pendulum_is_at_the_bottom_in_the_middle_of_a_swing);
  RUN_TEST(test_pendulum_swings_equally_to_both_sides);
  RUN_TEST(test_pendulum_moves_clockwise_first_then_back);
  RUN_TEST(test_pendulum_is_always_a_valid_led);
  RUN_TEST(test_fade_starts_dark);
  RUN_TEST(test_fade_full_at_the_end);
  RUN_TEST(test_fade_full_after_the_end);
  RUN_TEST(test_fade_is_already_rising_at_10_seconds);
  RUN_TEST(test_fade_halfway_value);
  RUN_TEST(test_fade_keeps_rising);
  RUN_TEST(test_settings_valid_are_kept);
  RUN_TEST(test_settings_virgin_ff_gets_defaults);
  RUN_TEST(test_settings_virgin_zero_gets_valid_led_offset);
  RUN_TEST(test_settings_clock_mode_bounds);
  RUN_TEST(test_settings_alarm_time_bounds);
  RUN_TEST(test_settings_alarm_set_flag);
  RUN_TEST(test_settings_every_alarm_mode_survives_reboot);
  RUN_TEST(test_settings_alarm_mode_out_of_range);
  RUN_TEST(test_settings_led_offset_both_valid);
  RUN_TEST(test_release_after_short_click_opens_alarm);
  RUN_TEST(test_release_after_hold_limit_opens_alarm);
  RUN_TEST(test_release_after_long_click_opens_clock_setting);
  RUN_TEST(test_release_without_press_at_boot_stays_in_clock);
  RUN_TEST(test_wrap_forward);
  RUN_TEST(test_wrap_forward_at_end);
  RUN_TEST(test_wrap_backward);
  RUN_TEST(test_wrap_backward_at_zero);
  RUN_TEST(test_wrap_hours);
  RUN_TEST(test_wrap_alarm_mode);
  RUN_TEST(test_countdown_add_minute);
  RUN_TEST(test_countdown_remove_minute);
  RUN_TEST(test_countdown_left_from_zero);
  RUN_TEST(test_countdown_right_from_59_gives_60);
  RUN_TEST(test_countdown_right_from_60_wraps_to_0);
  RUN_TEST(test_countdown_left_from_60_gives_59);
  RUN_TEST(test_countdown_left_then_right_returns_to_start);
  RUN_TEST(test_ramp_start);
  RUN_TEST(test_ramp_one_second);
  RUN_TEST(test_ramp_top_of_range);
  RUN_TEST(test_ramp_after_40_seconds_is_not_negative);
  RUN_TEST(test_ramp_after_40_seconds_value);
  RUN_TEST(test_ramp_after_2_minutes_keeps_growing);
  RUN_TEST(test_ramp_saturates_instead_of_wrapping);
  return UNITY_END();
}
