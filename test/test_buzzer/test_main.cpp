#include <unity.h>
#include "buzzer_sequencer.h"

void setUp() {}
void tearDown() {}

// Runs the sequencer in 1 ms steps and records the tone edges.
struct Trace {
  uint32_t on[256];  int nOn;
  uint32_t off[256]; int nOff;
  uint32_t finishedAt; int nFinished;
};

static void note(Trace& t, uint32_t now, BuzzerEvent e) {
  if (e.setTone && e.tone && t.nOn < 256) t.on[t.nOn++] = now;
  if (e.setTone && !e.tone && t.nOff < 256) t.off[t.nOff++] = now;
  if (e.finished) { t.finishedAt = now; t.nFinished++; }
}

static Trace run(const BuzzerPattern& p, uint32_t startMs, uint32_t durationMs) {
  Trace t = {};
  BuzzerSequencer s;
  note(t, startMs, sequencerStart(s, p, startMs));
  for (uint32_t d = 1; d <= durationMs; d++) note(t, startMs + d, sequencerTick(s, startMs + d));
  return t;
}

// ---- basics ----
void test_start_beeps_right_away() {
  BuzzerSequencer s;
  BuzzerEvent e = sequencerStart(s, ALARM_BEEP, 1000);
  TEST_ASSERT_TRUE(e.setTone); TEST_ASSERT_TRUE(e.tone); TEST_ASSERT_FALSE(e.finished); TEST_ASSERT_TRUE(s.running);
}
void test_nothing_happens_before_the_beep_ends() {
  BuzzerSequencer s; sequencerStart(s, ALARM_BEEP, 1000);
  BuzzerEvent e = sequencerTick(s, 1299);
  TEST_ASSERT_FALSE(e.setTone); TEST_ASSERT_FALSE(e.finished);
}
void test_tick_when_not_running_does_nothing() {
  BuzzerSequencer s;
  BuzzerEvent e = sequencerTick(s, 123456);
  TEST_ASSERT_FALSE(e.setTone); TEST_ASSERT_FALSE(e.finished);
}

// ---- alarm pattern: 3 beeps 300 on / 200 off, 1000 pause, endless ----
void test_alarm_first_group_timing() {
  Trace t = run(ALARM_BEEP, 0, 1400);
  TEST_ASSERT_EQUAL(0, t.on[0]);    TEST_ASSERT_EQUAL(300, t.off[0]);
  TEST_ASSERT_EQUAL(500, t.on[1]);  TEST_ASSERT_EQUAL(800, t.off[1]);
  TEST_ASSERT_EQUAL(1000, t.on[2]); TEST_ASSERT_EQUAL(1300, t.off[2]);
}
void test_alarm_repeats_after_the_pause() {
  Trace t = run(ALARM_BEEP, 0, 2500);
  TEST_ASSERT_EQUAL(2300, t.on[3]);  // 1300 + 1000 pause
}
void test_alarm_never_finishes() {
  Trace t = run(ALARM_BEEP, 0, 60000);
  TEST_ASSERT_EQUAL(0, t.nFinished);
  TEST_ASSERT_TRUE(t.nOn > 20);
}

// ---- timer pattern: 2 beeps 300 on / 500 off, 1000 pause, 5 cycles ----
void test_timer_beeps_ten_times() {
  Trace t = run(TIMER_BEEP, 0, 20000);
  TEST_ASSERT_EQUAL(10, t.nOn); TEST_ASSERT_EQUAL(10, t.nOff);
}
void test_timer_cycle_timing() {
  Trace t = run(TIMER_BEEP, 0, 4000);
  TEST_ASSERT_EQUAL(0, t.on[0]);    TEST_ASSERT_EQUAL(300, t.off[0]);
  TEST_ASSERT_EQUAL(800, t.on[1]);  TEST_ASSERT_EQUAL(1100, t.off[1]);
  TEST_ASSERT_EQUAL(2100, t.on[2]);  // 1100 + 1000 pause
}
void test_timer_finishes_exactly_once_when_the_last_beep_ends() {
  Trace t = run(TIMER_BEEP, 0, 20000);
  TEST_ASSERT_EQUAL(1, t.nFinished);
  TEST_ASSERT_EQUAL(9500, t.finishedAt);      // 4 * 2100 + 800 + 300
  TEST_ASSERT_EQUAL(9500, t.off[9]);          // and the tone is switched off at the same time
}
void test_nothing_after_finished() {
  BuzzerSequencer s;
  sequencerStart(s, TIMER_BEEP, 0);
  for (uint32_t t = 1; t <= 9500; t++) sequencerTick(s, t);
  TEST_ASSERT_FALSE(s.running);
  BuzzerEvent e = sequencerTick(s, 50000);
  TEST_ASSERT_FALSE(e.setTone); TEST_ASSERT_FALSE(e.finished);
}
void test_can_start_again_after_finishing() {
  BuzzerSequencer s;
  sequencerStart(s, TIMER_BEEP, 0);
  for (uint32_t t = 1; t <= 9500; t++) sequencerTick(s, t);
  BuzzerEvent e = sequencerStart(s, ALARM_BEEP, 20000);
  TEST_ASSERT_TRUE(e.tone); TEST_ASSERT_TRUE(s.running);
}

// ---- stop ----
void test_stop_in_the_middle_of_a_beep() {
  BuzzerSequencer s; sequencerStart(s, ALARM_BEEP, 0);
  sequencerTick(s, 100);
  sequencerStop(s);
  TEST_ASSERT_FALSE(s.running); TEST_ASSERT_FALSE(s.toneOn);
  BuzzerEvent e = sequencerTick(s, 5000);
  TEST_ASSERT_FALSE(e.setTone); TEST_ASSERT_FALSE(e.finished);
}
void test_stopped_timer_does_not_report_finished() {
  BuzzerSequencer s; sequencerStart(s, TIMER_BEEP, 0);
  sequencerStop(s);
  TEST_ASSERT_FALSE(sequencerTick(s, 10000).finished);
}

// ---- time behaviour ----
void test_millis_rollover_in_the_middle_of_a_beep() {
  uint32_t start = 0xFFFFFF00u;  // 256 ms before rollover; the beep ends 44 ms after it
  BuzzerSequencer s; sequencerStart(s, ALARM_BEEP, start);
  TEST_ASSERT_FALSE(sequencerTick(s, 0xFFFFFFFFu).setTone);
  TEST_ASSERT_FALSE(sequencerTick(s, 43).setTone);
  BuzzerEvent e = sequencerTick(s, 44);
  TEST_ASSERT_TRUE(e.setTone); TEST_ASSERT_FALSE(e.tone);
}
void test_full_pattern_across_the_rollover() {
  Trace t = run(TIMER_BEEP, 0xFFFFF000u, 20000);
  TEST_ASSERT_EQUAL(10, t.nOn); TEST_ASSERT_EQUAL(1, t.nFinished);
}
void test_late_tick_is_handled_once_and_next_deadline_follows_the_actual_time() {
  BuzzerSequencer s; sequencerStart(s, ALARM_BEEP, 0);   // beep should end at 300
  BuzzerEvent e = sequencerTick(s, 340);                  // a slow loop (e.g. LED update) notices it late
  TEST_ASSERT_TRUE(e.setTone); TEST_ASSERT_FALSE(e.tone);
  TEST_ASSERT_FALSE(sequencerTick(s, 539).setTone);      // 340 + 200 off
  TEST_ASSERT_TRUE(sequencerTick(s, 540).setTone);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_start_beeps_right_away);
  RUN_TEST(test_nothing_happens_before_the_beep_ends);
  RUN_TEST(test_tick_when_not_running_does_nothing);
  RUN_TEST(test_alarm_first_group_timing);
  RUN_TEST(test_alarm_repeats_after_the_pause);
  RUN_TEST(test_alarm_never_finishes);
  RUN_TEST(test_timer_beeps_ten_times);
  RUN_TEST(test_timer_cycle_timing);
  RUN_TEST(test_timer_finishes_exactly_once_when_the_last_beep_ends);
  RUN_TEST(test_nothing_after_finished);
  RUN_TEST(test_can_start_again_after_finishing);
  RUN_TEST(test_stop_in_the_middle_of_a_beep);
  RUN_TEST(test_stopped_timer_does_not_report_finished);
  RUN_TEST(test_millis_rollover_in_the_middle_of_a_beep);
  RUN_TEST(test_full_pattern_across_the_rollover);
  RUN_TEST(test_late_tick_is_handled_once_and_next_deadline_follows_the_actual_time);
  return UNITY_END();
}
