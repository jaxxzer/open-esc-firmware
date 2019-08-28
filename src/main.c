#include <bridge.h>
#include <comparator.h>
#include <console.h>
#include <pwm-input.h>

#include "debug.h"

#include <libopencm3/cm3/vector.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

volatile bool starting;

uint32_t zc_confirmations_required = 10;
const int16_t tim15psc = 5;
volatile uint32_t zc_counter;
volatile uint8_t advance = 0;

const uint16_t defaultCCR1 = 10000;
const uint16_t defaultCCR2 = defaultCCR1 + defaultCCR1/8;

// void led_toggle() {
//   gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
// }

void tim15_isr() {
  // check tim15 cc1 interrupt
  if (timer_get_flag(TIM15, TIM_SR_CC1IF)) {

    debug1_toggle();
    bridge_commutate();
    comparator_set_state(g_bridge_comm_step + 2);
    timer_clear_flag(TIM15, TIM_SR_CC1IF);
  }
  // check tim15 cc2 interrupt
  // ccr2 - ccr1 = blanking period
  if (timer_get_flag(TIM15, TIM_SR_CC2IF)) {
    zc_counter = zc_confirmations_required;
    comparator_zc_isr_enable();
    timer_clear_flag(TIM15, TIM_SR_CC2IF);
  }
}

// for test
// void comparator_zc_isr()
// {
//   comparator_blank(50000);
//   comparator_set_state(g_comparator_state + 1);
//   gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
// }


void comparator_zc_isr()
{
  if (TIM_CNT(TIM1) < 120) {
    return;
  }

  if (zc_counter--) {
    return;
  }

  uint16_t cnt = TIM_CNT(TIM15);
  TIM_CNT(TIM15) = 0;



  if (starting) {
    starting = false;
    debug2_toggle();
    debug2_toggle();
    TIM_CCR1(TIM15) = cnt;
  } else {
    if (cnt < TIM_CCR1(TIM15)) {
      starting = true;
      debug0_toggle();
      debug0_toggle();
      TIM_CCR1(TIM15) = defaultCCR1;
    } else {
      debug2_toggle();
      TIM_CCR1(TIM15) = cnt/2;
    }
  }

  TIM_CCR2(TIM15) = TIM_CCR1(TIM15) + TIM_CCR1(TIM15)/8;

  // TIM_CCR2(TIM15) = 800;
  comparator_zc_isr_disable();
  zc_counter = zc_confirmations_required;
}

void commutation_timer_enable_interrupts()
{
  nvic_enable_irq(NVIC_TIM15_IRQ);
  TIM_DIER(TIM15) |= TIM_DIER_CC1IE | TIM_DIER_CC2IE;
}

void commutation_timer_disable_interrupts()
{
  TIM_DIER(TIM15) &= ~(TIM_DIER_CC1IE | TIM_DIER_CC2IE);
  nvic_disable_irq(NVIC_TIM15_IRQ);
}

void stop_motor()
{
  commutation_timer_disable_interrupts();
  comparator_zc_isr_disable();
}

void start_motor()
{
  debug0_toggle();
  starting = true;
  TIM_CR1(TIM15) &= ~TIM_CR1_CEN; // disable counter
  TIM_CNT(TIM15) = 0; // set counter to zero
  TIM_CCR1(TIM15) = defaultCCR1;
  TIM_CCR2(TIM15) = defaultCCR2;
  zc_counter = zc_confirmations_required;

  commutation_timer_enable_interrupts();
  //comparator_zc_isr_enable();

  g_bridge_comm_step = BRIDGE_COMM_STEP0;
  comparator_set_state(g_bridge_comm_step);

  bridge_set_run_duty(100);
  TIM_CR1(TIM15) |= TIM_CR1_CEN; // enable counter
}

void commutation_timer_setup()
{
  rcc_periph_clock_enable(RCC_TIM15);
  TIM_ARR(TIM15) = 0xffff;
  TIM_PSC(TIM15) = tim15psc;
}

int main()
{
  debug_setup();

  //while(1);
  bridge_initialize();
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_AUDIO);
  bridge_set_audio_duty(0x1);

  for (int j = 0; j < 10; j++) {
  bridge_set_audio_frequency(8000);
  for (uint32_t i = 0; i < 5000; i++) { float a = 0.6*9; }
  bridge_set_audio_frequency(3000);
  for (uint32_t i = 0; i < 10000; i++) { float a = 0.6*9; }
  }
  bridge_set_audio_frequency(3500);
  for (uint32_t i = 0; i < 60000; i++) { float a = 0.6*9; }

  bridge_disable();

  for (int i = 0; i < 9999; i++) { float a = 0.6*9; }

  comparator_initialize();

  // for test
  // comparator_zc_isr_enable();
  // while(1);

  commutation_timer_setup();
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_RUN);
  for (uint8_t i = 0; i < 6; i++) {
    start_motor();
    advance = i;
    for (uint32_t i = 0; i < 300000; i++) { float a = 0.6*9; }
    stop_motor();
  }
  bridge_disable();
  stop_motor();

  while(1);


  //console_write("waiting for pwm input...\r\n");


  uint8_t succeded = 0;
  const uint8_t succeded_needed = 3;

  while (1) {
    // gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
    pwm_input_type_t pwm_type_last = g_pwm_type;
    while (!timer_get_flag(PWM_INPUT_TIMER, TIM_SR_UIF)) {
    }

    timer_clear_flag(PWM_INPUT_TIMER, TIM_SR_UIF);

    g_pwm_type = pwm_input_detect_type();

    if (g_pwm_type == PWM_INPUT_TYPE_NONE || g_pwm_type == PWM_INPUT_TYPE_UNKNOWN) {
      succeded = 0;
      continue;
    }

    if (g_pwm_type != pwm_type_last && succeded != 0) {
      succeded = 0;
      continue;
    }

    if (++succeded == succeded_needed) {
      break;
    }
  }

  while (1) {
    gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
    console_write_pwm_info();
  }
}
