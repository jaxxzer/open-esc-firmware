
extern "C" {
#include <adc.h>
#include <bemf.h>
#include <bridge.h>
#include <commutation.h>
#include <comparator.h>
#include <console.h>
#include <debug-pins.h>
#include <global.h>
#include <isr.h>
#include <led.h>
#include <overcurrent-watchdog.h>
#include <pwm-input.h>
#include <system.h>
#include <watchdog.h>
}

// hpp
#include <audio.h>
#include <io.h>

#include <target.h>

// use this throttle in open loop startup
const uint16_t startup_throttle = 75;

// declared/documented in global.h
uint16_t run_zc_confirmations_required = slow_run_zc_confirmations_required;
const uint16_t slow_run_zc_confirmations_required = 14;
volatile bool starting;
const uint16_t startup_commutation_period_ticks = 12000;
const uint16_t startup_zc_confirmations_required = 15;
volatile uint32_t zc_counter;
uint32_t zc_confirmations_required = startup_zc_confirmations_required;

// for test
void test_spin()
{
  for (uint32_t i = 0; i < 4000; i++) { io_write_state(); watchdog_reset(); }
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_RUN);
  bridge_set_run_duty(200);
  watchdog_reset();
  start_motor();
  for (uint32_t i = 200; i < 500; i++) {
    for (uint32_t j = 0; j < 100; j++) {
      watchdog_reset();
      bridge_set_run_duty(i);
      io_write_state();
    }
  }

  bridge_disable();
  stop_motor();

  while(1) {watchdog_reset();}
}

int main()
{
  system_initialize();

  io_initialize();

  led_initialize();

  debug_pins_initialize();

  console_initialize();
  console_write("welcome to open-esc!\r\n");

  for (uint32_t j = 0; j < 1000000; j++) { float a = 0.6*9; }

  adc_initialize();
  overcurrent_watchdog_initialize();
  adc_start();

  watchdog_start(10); // 10ms watchdog timeout

  bridge_initialize();
  bridge_enable_adc_trigger();

  // startup beep
  audio_play_note_blocking(1000, 0x04, 40000);
  audio_play_note_blocking(1200, 0x04, 40000);
  audio_play_note_blocking(1600, 0x04, 40000);
  for (int i = 0; i < 9999; i++) { watchdog_reset(); io_process_input(); }

#if defined(FEEDBACK_COMPARATOR)
  // initialize comparator
  comparator_initialize();
#elif defined(FEEDBACK_BEMF)
  // initialize bemf
  bemf_initialize();
#else
#error "no feedback defined"
#endif

  // initialize motor run timers
  commutation_timer_initialize();
  zc_timer_initialize();

  // detect pwm input type
  pwm_input_initialize();
  // successive pwm input type detection checks passed
  uint8_t succeded = 0;
  // successive pwm input type detection checks needed
  const uint8_t succeded_needed = 3;
  pwm_input_type_t pwm_type_check;

  // setup input signal timer with 25ms timeout
  // todo: rework pwm_input, throttle, and io interfaces for some sense
  pwm_input_set_type(PWM_INPUT_TYPE_CONVENTIONAL);
  while (!g.throttle_valid)
  {
    watchdog_reset();
    led_toggle();
    io_process_input();
    // todo beep once in a while
  }

  // pwm input type valid confirmation beep
  audio_play_note_blocking(1000, 0x04, 90000);

  // wait for low throttle
  while (g.throttle > 0) {
    watchdog_reset();
    io_process_input();
  }

  // low throttle armed beep
  audio_play_note_blocking(1600, 0x04, 90000);

  // prepare the motor for run mode (armed)
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_RUN);

  // we are armed

  // initialize commutation and zc timers for open loop
  // motor won't move until throttle is applied
  start_motor();

  while(1) {
    watchdog_reset();
    led_toggle();
    io_process_input();
    if (g.throttle_valid && g.throttle > startup_throttle) {
      bridge_enable();
      bridge_set_run_duty(g_bridge_run_duty/2 + g.throttle/2);
    } else {
      bridge_disable();
    }
  }

  // disable the motor output
  bridge_disable();
  // stop commutation and zc timers
  stop_motor();

  // independent watchdog will issue a system reset
  while (1) {};
}
