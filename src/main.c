#include <bridge.h>
#include <comparator.h>
#include <console.h>
#include <pwm-input.h>

#include <libopencm3/cm3/vector.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

void tim15_isr() {
  if (timer_get_flag(TIM15, TIM_SR_CC1IF)) {
    bridge_commutate();
    comparator_set_state(g_bridge_comm_step+4);
    timer_clear_flag(TIM15, TIM_SR_CC1IF);
  } else if (timer_get_flag(TIM15, TIM_SR_CC2IF)) {
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

uint16_t zc_confirmations_required = 5;
volatile uint16_t zc_counter;

void comparator_zc_isr()
{
  if (zc_counter--) {
    return;
  }

  uint16_t cnt = TIM_CNT(TIM15);
  TIM_CNT(TIM15) = 0;

  if (cnt < 4000) {
    TIM_CCR1(TIM15) = 2000;
  } else {
    TIM_CCR1(TIM15) = cnt/2;
  }

  TIM_CCR2(TIM15) = TIM_CCR1(TIM15) + TIM_CCR1(TIM15)/2;

  // TIM_CCR2(TIM15) = 800;
  comparator_zc_isr_disable();
  gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
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
  bridge_disable();
  commutation_timer_disable_interrupts();
  comparator_zc_isr_disable();
}

void start_motor()
{
  TIM_CR1(TIM15) &= ~TIM_CR1_CEN; // disable counter
  TIM_CNT(TIM15) = 0; // set counter to zero
  TIM_CCR1(TIM15) = 40000;
  TIM_CCR2(TIM15) = 60000;
  zc_counter = zc_confirmations_required;

  commutation_timer_enable_interrupts();
  //comparator_zc_isr_enable();

  g_bridge_comm_step = BRIDGE_COMM_STEP0;
  comparator_set_state(g_bridge_comm_step);

  bridge_set_run_duty(90);
  TIM_CR1(TIM15) |= TIM_CR1_CEN; // enable counter
}

void commutation_timer_setup()
{
  rcc_periph_clock_enable(RCC_TIM15);
  TIM_ARR(TIM15) = 0xffff;
  TIM_PSC(TIM15) = 4;
}



int main()
{
  //console_initialize();
  //console_write("\r\nWelcome to gsc: the gangster esc!\r\n");

  //console_write("testing bridge...\r\n");

  bridge_initialize();
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_AUDIO);
  bridge_set_audio_duty(0x8);

  bridge_set_audio_frequency(800);
  for (uint32_t i = 0; i < 60000; i++) { i -= 1; float a = 0.6*9;i += 1; }
  bridge_set_audio_frequency(2000);
  for (uint32_t i = 0; i < 60000; i++) { float a = 0.6*9; }
  bridge_set_audio_frequency(3500);
  for (uint32_t i = 0; i < 60000; i++) { float a = 0.6*9; }

  bridge_disable();

  for (int i = 0; i < 9999; i++) { float a = 0.6*9; }

  //while(1);
  
  //console_write("initializing comparator...\r\n");

  comparator_initialize();

  pwm_input_initialize();

  rcc_periph_clock_enable(LED_GPIO_RCC);

  gpio_mode_setup(LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GPIO_PIN);

  // for test
  // comparator_zc_isr_enable();
  // while(1);


  commutation_timer_setup();
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_RUN);
  start_motor();
  for (uint32_t i = 0; i < 180000; i++) { float a = 0.6*9; }
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
