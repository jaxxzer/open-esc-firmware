#pragma once

#include <inttypes.h>

typedef enum {
    BRIDGE_STATE_DISABLED, // aka freewheel
    BRIDGE_STATE_AUDIO, // arr is adjusted for audio frequency
    BRIDGE_STATE_RUN, // arr is fixed at 2048 for throttle resolution
} bridge_state_e;

typedef enum {
    BRIDGE_COMM_STEP0,
    BRIDGE_COMM_STEP1,
    BRIDGE_COMM_STEP2,
    BRIDGE_COMM_STEP3,
    BRIDGE_COMM_STEP4,
    BRIDGE_COMM_STEP5,
} bridge_comm_step_e;

volatile bridge_state_e g_bridge_state;
volatile bridge_comm_step_e g_bridge_comm_step;

volatile uint16_t g_bridge_run_duty; // run mode duty
volatile uint8_t g_bridge_audio_duty; // audio mode duty

void bridge_initialize();
void bridge_set_state(bridge_state_e state);

void bridge_enable();
void bridge_disable();

// always 4 bit
void bridge_set_audio_duty(uint8_t duty);

void bridge_set_audio_frequency(uint16_t frequency);

// always 12 bit (results in ~21kHz pwm @ 48Mhz)
void bridge_set_run_duty(uint16_t duty);

void bridge_commutate();
