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

int main() {
  UNITY_BEGIN();
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
