#pragma once

#include <inttypes.h>
#define ADC_MAX_CHANNELS 7

volatile uint16_t adc_buffer[ADC_MAX_CHANNELS];
void adc_initialize();
void adc_start();
volatile uint16_t adc_read_channel(uint8_t channel);
