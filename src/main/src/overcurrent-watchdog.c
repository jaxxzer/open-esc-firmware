#include <overcurrent-watchdog.h>
#include <target.h>
#include <bridge.h>
#include <commutation.h>
#include <target.h>
#include <watchdog.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/adc.h>

void overcurrent_watchdog_initialize()
{
  // set AWD high threshold
  ADC_TR1(ADC1) = (ADC_WWDG_CURRENT_MAX << 16) & 0x0fff0000;

  // enable watchdog on selected channel
  ADC_CFGR1(ADC1) = (ADC_CFGR1(ADC1) & ~ADC_CFGR1_AWD1CH) | ADC_CFGR1_AWD1CH_VAL(ADC_CHANNEL_BUS_CURRENT);
  ADC_CFGR1(ADC1) |= ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL;

  // enable watchdog interrupt
  ADC_IER(ADC1) |= ADC_IER_AWD1IE;
  nvic_enable_irq(OVERCURRENT_WATCHDOG_IRQ);
}

void overcurrent_watchdog_isr()
{
  bridge_disable();
  stop_motor();
  while (1) { watchdog_reset(); }
}
