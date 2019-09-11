
#include <isr.h>

#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

void tim6_dac_isr()
{
  // timeout waiting for zero-cross, go to open loop
  // TODO should load commutation timer default values
  if (timer_get_flag(ZC_TIMER, TIM_SR_UIF)) {
    comparator_zc_timeout_isr();
    timer_clear_flag(ZC_TIMER, TIM_SR_UIF);
  }
}

void adc_comp_isr()
{
  if (exti_get_flag_status(EXTI21)) {
    comparator_zc_isr(); // user code
    exti_reset_request(EXTI21);
  }
}

void tim16_isr()
{
  if (timer_get_flag(COMMUTATION_TIMER, TIM_SR_UIF)) {
    commutation_isr();
    timer_clear_flag(COMMUTATION_TIMER, TIM_SR_UIF);
  }

  // check cc1 interrupt
  // ccr1 = comparator blanking period
  if (timer_get_flag(COMMUTATION_TIMER, TIM_SR_CC1IF)) {
    comparator_unblank_isr();
    timer_clear_flag(COMMUTATION_TIMER, TIM_SR_CC1IF);
  }
}

void tim17_isr() {
    comparator_blank_complete_isr();
    timer_clear_flag(TIM17, TIM_SR_UIF);
}
