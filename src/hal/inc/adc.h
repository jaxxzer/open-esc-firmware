#pragma once

#include <inttypes.h>

void adc_initialize();
void adc_start();
volatile uint16_t adc_read_channel(uint8_t channel);
