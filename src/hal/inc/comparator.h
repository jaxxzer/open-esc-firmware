// single comparator driver

#pragma once

#include <inttypes.h>

typedef enum {
    COMP_STATE0,
    COMP_STATE1,
    COMP_STATE2,
    COMP_STATE3,
    COMP_STATE4,
    COMP_STATE5,
} comp_state_e;

volatile comp_state_e g_comparator_state;

void comparator_initialize();

void comparator_set_state(comp_state_e new_state);

void comparator_zc_isr_enable();
void comparator_zc_isr_disable();

// comparator bemf zero cross isr, normally you will commutate
void comparator_zc_isr();

void comparator_blank(uint32_t nanoseconds);
