#include <watchdog.h>

#include <libopencm3/stm32/iwdg.h>

void watchdog_start(uint32_t period_ms)
{
    iwdg_set_period_ms(period_ms);
    iwdg_start();
}

void watchdog_reset()
{
    iwdg_reset();
}
