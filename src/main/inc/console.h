#pragma once

#include <inttypes.h>

void console_initialize();
void console_write(const char* string);
void console_write_int(uint32_t i);

void console_write_pwm_info();
