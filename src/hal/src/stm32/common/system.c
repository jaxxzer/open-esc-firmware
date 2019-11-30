#include <system.h>

#include <libopencm3/stm32/rcc.h>

void system_initialize()
{
  system_clock_initialize();
}

void system_clock_initialize()
{
#if defined(STM32F0)
  rcc_clock_setup_in_hsi_out_48mhz();
#elif defined(STM32F3)
  rcc_clock_setup_pll(&rcc_hsi_configs[1]);
#endif
}
