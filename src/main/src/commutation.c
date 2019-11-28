#include <commutation.h>

#include <bridge.h>
#include <comparator.h>
#include <global.h>

#include <target.h>

#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

// commutation and zero cross timers must have the same frequency
const uint32_t commutation_zc_timer_frequency = 2000000;

void commutation_timer_enable_interrupts()
{
  nvic_enable_irq(COMMUTATION_TIMER_IRQ);
  TIM_DIER(COMMUTATION_TIMER) |= TIM_DIER_CC1IE | TIM_DIER_UIE;
}

void zc_timer_enable_interrupts()
{
  nvic_enable_irq(ZC_TIMER_IRQ);
  TIM_DIER(ZC_TIMER) |= TIM_DIER_UIE;
}

void commutation_timer_disable_interrupts()
{
  TIM_DIER(COMMUTATION_TIMER) &= ~(TIM_DIER_CC1IE | TIM_DIER_UIE);
  nvic_disable_irq(COMMUTATION_TIMER_IRQ);
}

void zc_timer_disable_interrupts()
{
  TIM_DIER(ZC_TIMER) &= ~(TIM_DIER_UIE);
  nvic_disable_irq(ZC_TIMER_IRQ);
}

void stop_motor()
{
  commutation_timer_disable_interrupts();
  comparator_zc_isr_disable();
  zc_timer_disable_interrupts();
}

void start_motor()
{
  starting = true;
  TIM_CR1(COMMUTATION_TIMER) &= ~TIM_CR1_CEN; // disable counter
  TIM_CNT(COMMUTATION_TIMER) = 1; // set counter to zero
  TIM_ARR(COMMUTATION_TIMER) = startup_commutation_period_ticks;
  TIM_CCR1(COMMUTATION_TIMER) = TIM_ARR(COMMUTATION_TIMER)/16;
  zc_counter = zc_confirmations_required;

  g_bridge_comm_step = BRIDGE_COMM_STEP0;
  comparator_set_state((comp_state_e)g_bridge_comm_step);

  commutation_timer_enable_interrupts();
  zc_timer_enable_interrupts();
  //bridge_set_run_duty(startup_throttle);
  TIM_CR1(COMMUTATION_TIMER) |= TIM_CR1_CEN; // enable counter
  TIM_CNT(ZC_TIMER) = TIM_ARR(COMMUTATION_TIMER)/2;
  TIM_CR1(ZC_TIMER) |= TIM_CR1_CEN; // enable counter
}

void commutation_timer_initialize()
{
  rcc_periph_clock_enable(COMMUTATION_TIMER_RCC);
  TIM_ARR(COMMUTATION_TIMER) = 0xffff;
  TIM_PSC(COMMUTATION_TIMER) = (rcc_ahb_frequency / commutation_zc_timer_frequency) - 1;
  // prescaler is not applied until update event
  TIM_EGR(COMMUTATION_TIMER) |= TIM_EGR_UG;
  TIM_SR(COMMUTATION_TIMER) = 0;
}

void zc_timer_initialize()
{
  rcc_periph_clock_enable(ZC_TIMER_RCC);
  TIM_ARR(ZC_TIMER) = 0xffff;
  TIM_PSC(ZC_TIMER) = (rcc_ahb_frequency / commutation_zc_timer_frequency) - 1;
  // prescaler is not applied until update event
  TIM_EGR(ZC_TIMER) |= TIM_EGR_UG;
  TIM_SR(ZC_TIMER) = 0;

  // commutation timer takes priority over comparator
  nvic_set_priority(ZC_TIMER_IRQ, 0x40);
}
