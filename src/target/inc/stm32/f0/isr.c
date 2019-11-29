
#include <adc.h>
#include <isr.h>

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

#include <target.h>

void tim14_isr()
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
  if (adc_get_watchdog_flag(ADC1)) {
    overcurrent_watchdog_isr();
    adc_clear_watchdog_flag(ADC1);
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

void tim1_cc_isr() {
  if (timer_get_flag(TIM1, TIM_SR_CC4IF)) {
    adc_start();
    timer_clear_flag(TIM1, TIM_SR_CC4IF);
  }
}

void dma1_channel4_7_dma2_channel3_5_isr() {
    usart_dma_transfer_complete_isr(CONSOLE_USART);
    // clear isr flag
    DMA_IFCR(CONSOLE_TX_DMA) |= DMA_IFCR_CTCIF(CONSOLE_TX_DMA_CHANNEL);
}

void dma1_channel2_3_dma2_channel1_2_isr() {
    usart_dma_transfer_complete_isr(CONSOLE_USART);
    // clear isr flag
    DMA_IFCR(CONSOLE_TX_DMA) |= DMA_IFCR_CTCIF(CONSOLE_TX_DMA_CHANNEL);
}

void tim2_isr() {
  io_control_timeout_isr();
  timer_clear_flag(TIM2, TIM_SR_UIF);
}

void tim15_isr() {
  io_control_timeout_isr();
  timer_clear_flag(TIM15, TIM_SR_UIF);
}

void tim3_isr() {
  io_control_timeout_isr();
  timer_clear_flag(TIM3, TIM_SR_UIF);
}
