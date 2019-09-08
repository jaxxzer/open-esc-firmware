#pragma once

#include <inttypes.h>

void watchdog_start(uint32_t period_ms);
void watchdog_reset();
