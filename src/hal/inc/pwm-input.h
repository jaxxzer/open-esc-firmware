#pragma once

#include <inttypes.h>
#include <stdbool.h>

#define PERIOD_MAX_NS 22000000 // 50Hz - 10%

// dshot150 is 150kbps
#define DIGITAL_PERIOD_MAX_NS 28409 // ie 'bits' incoming faster than 32kHz + 10%

#define MULTISHOT_DUTY_MAX_NS 27500// 25us + 10%
#define ONESHOT42_DUTY_MAX_NS 92400 // 84us + 10%
#define ONESHOT125_DUTY_MAX_NS 275000 // 250us + 10%
#define CONVENTIONAL_DUTY_MAX_NS 2200000 // 2000us + 10%

typedef enum {
    PWM_INPUT_TYPE_NONE,
    PWM_INPUT_TYPE_CONVENTIONAL,
    PWM_INPUT_TYPE_ONESHOT125,
    PWM_INPUT_TYPE_ONESHOT42,
    PWM_INPUT_TYPE_MULTISHOT,
    PWM_INPUT_TYPE_UNKNOWN,
} pwm_input_type_t;

pwm_input_type_t g_pwm_type;

void pwm_input_initialize();
void pwm_input_deinitialize();

// set the timeout in microseconds from last valid signal
void pwm_input_set_timeout(uint32_t timeout);
uint32_t pwm_input_get_timeout();

pwm_input_type_t pwm_input_detect_type();

uint32_t pwm_input_tick_period_ns();

// period in ticks
uint32_t pwm_input_get_period();
uint32_t pwm_input_get_period_ns();

// duty in ticks
uint32_t pwm_input_get_duty();
uint32_t pwm_input_get_duty_ns();

void pwm_input_set_type(pwm_input_type_t input_type);

// 16-bit throttle
uint16_t pwm_input_get_throttle();

// still valid?
// valid if:
// - the signal is in the expected range for the selecte type
// - the timeout period has not elapsed since the last valid signal
bool pwm_input_valid();
