#pragma once

#include <inttypes.h>

void led_initialize();
void led_on();
void led_off();
void led_toggle();
void led_write(uint8_t r, uint8_t g, uint8_t b);
