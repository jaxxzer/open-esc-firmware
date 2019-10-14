#pragma once

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
