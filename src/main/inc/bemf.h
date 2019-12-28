#pragma once

#include <inttypes.h>

typedef enum {
    BEMF_PHASE_A,
    BEMF_PHASE_B,
    BEMF_PHASE_C,
    BEMF_PHASE_N,
} bemf_phase_e;

typedef enum {
    BEMF_STEP0,
    BEMF_STEP1,
    BEMF_STEP2,
    BEMF_STEP3,
    BEMF_STEP4,
    BEMF_STEP5,
} bemf_step_e;

volatile bemf_step_e g_bemf_step;

void bemf_disable_divider();
void bemf_enable_divider();

// return voltage in millivolts (~65V  6s or so max for now)
uint16_t bemf_get_phase_voltage(bemf_phase_e phase);
uint16_t bemf_get_phase_adc_raw(bemf_phase_e phase);

void bemf_set_step(bemf_step_e new_step);
void bemf_blank(uint32_t nanoseconds);
void bemf_unblank_isr();
void bemf_initialize();
void bemf_zc_isr();
void bemf_zc_isr_enable();
void bemf_zc_isr_disable();
