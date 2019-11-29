#pragma once

// todo move this file out of hal and into application src

#include <inttypes.h>
#include <stdbool.h>

#define ADC_NUM_CHANNELS 8
typedef struct {
  uint16_t adc_buffer[ADC_NUM_CHANNELS];
  uint16_t throttle;
  uint8_t direction;
  uint8_t direction_mode;
  uint16_t startup_throttle;
  // if pwm signal is present at boot, it will be selected as the throttle signal
  // source and throttle commands from the serial interface will be ignored
  // if only serial is present, it will be selected and pwm input will be disabled.
  bool throttle_valid;
} global_t;

extern global_t g;

// the zero crosses required in closed loop mode, this is variable depending on current speed
extern uint16_t run_zc_confirmations_required;

// if true, we are in open-loop
// we wait for the first zero cross period (2 sequential valid zero crosses)
// timing, then we will enter closed loop
extern volatile bool starting;

extern const uint16_t slow_run_zc_confirmations_required;

// open loop startup commutation timer ARR value
// TODO do this in human-readable time (microseconds)
extern const uint16_t startup_commutation_period_ticks;

// the comparator output must hold for this many checks in a row before we consider it a valid zero-cross
// this can be descreased as rpm increases
extern const uint16_t startup_zc_confirmations_required;

// REMAINING zero cross checks before we pass
extern volatile uint32_t zc_counter;

// the zero cross confirmations currently required to pass a zero cross check
extern uint32_t zc_confirmations_required;
