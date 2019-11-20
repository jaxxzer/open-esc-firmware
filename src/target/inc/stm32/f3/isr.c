#include <adc.h>
#include <isr.h>
#include <throttle.h>

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

#include <target.h>

void comp123_isr()
{
  if (exti_get_flag_status(EXTI21)) {
    comparator_zc_isr(); // user code
    exti_reset_request(EXTI21);
  }
}

void tim14_isr()
{
  // timeout waiting for zero-cross, go to open loop
  // TODO should load commutation timer default values
  if (timer_get_flag(ZC_TIMER, TIM_SR_UIF)) {
    comparator_zc_timeout_isr();
    timer_clear_flag(ZC_TIMER, TIM_SR_UIF);
  }
}

void tim1_up_tim16_isr()
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

void tim1_trg_com_tim17_isr() {
    comparator_blank_complete_isr();
    timer_clear_flag(TIM17, TIM_SR_UIF);
}

void dma1_channel7_isr() {
  if (DMA_ISR(CONSOLE_TX_DMA) & DMA_ISR_TCIF(CONSOLE_TX_DMA_CHANNEL)) {
    usart_dma_transfer_complete_isr(CONSOLE_USART);
    // clear isr flag
    DMA_IFCR(CONSOLE_TX_DMA) |= DMA_IFCR_CTCIF(CONSOLE_TX_DMA_CHANNEL);
  }
}

void adc1_2_isr() {
  if (ADC_ISR(ADC1) & ADC_ISR_AWD1) {
    overcurrent_watchdog_isr();
    ADC_ISR(ADC1) = ADC_ISR_AWD1;
  }
}

void tim1_cc_isr() {
  if (timer_get_flag(TIM1, TIM_SR_CC4IF)) {
    adc_start();
    timer_clear_flag(TIM1, TIM_SR_CC4IF);
  }
}

void tim2_isr() {
  throttle_timeout_isr();
  timer_clear_flag(TIM2, TIM_SR_UIF);
}
