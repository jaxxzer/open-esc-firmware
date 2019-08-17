#pragma once

#include <inttypes.h>
#include <stdbool.h>

typedef enum {
    PWM_INPUT_TYPE_CONVENTIONAL,
    PWM_INPUT_TYPE_ONESHOT125,
    PWM_INPUT_TYPE_ONESHOT42,
    PWM_INPUT_TYPE_MULTISHOT,
    PWM_INPUT_TYPE_UNKNOWN,
} pwm_input_type_t;

void pwm_input_initialize();
void pwm_input_deinitialize();

// set the timeout in microseconds from last valid signal
void pwm_input_set_timeout(uint32_t timeout);
uint32_t pwm_input_get_timeout();

pwm_input_type_t pwm_input_detect_type();

// period in ticks
uint32_t pwm_input_get_period();

// duty in ticks
uint32_t pwm_input_get_duty();

void pwm_input_set_type(pwm_input_type_t input_type);
pwm_input_type_t pwm_input_get_type();


// still valid?
// valid if:
// - the signal is in the expected range for the selecte type
// - the timeout period has not elapsed since the last valid signal
bool pwm_input_valid();
