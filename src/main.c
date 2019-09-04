#include <bridge.h>
#include <comparator.h>
#include <console.h>
#include <pwm-input.h>

#include "main/inc/debug-pins.h"

#include <libopencm3/cm3/vector.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/comparator.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

// requires at least one compare channel for comparator blanking
#define COMMUTATION_TIMER TIM16
#define COMMUTATION_TIMER_RCC RCC_TIM16
#define COMMUTATION_TIMER_IRQ NVIC_TIM16_IRQ
#define ZC_TIMER TIM6
#define ZC_TIMER_RCC RCC_TIM6
#define ZC_TIMER_IRQ NVIC_TIM6_DAC_IRQ

// if true, we are in open-loop
// we wait for the first zero cross period (2 sequential valid zero crosses)
// timing, then we will enter closed loop
volatile bool starting;

// use this throttle in open loop startup
const uint16_t startup_throttle = 150; // ~7%

// the comparator output must hold for this many checks in a row before we consider it a valid zero-cross
// this can be descreased as rpm increases
const uint16_t startup_zc_confirmations_required = 15;
const uint16_t slow_run_zc_confirmations_required = 10;

// the zero crosses required in closed loop mode, this is variable depending on current speed
uint16_t run_zc_confirmations_required = slow_run_zc_confirmations_required;

// the zero cross confirmations currently required to pass a zero cross check
uint32_t zc_confirmations_required = startup_zc_confirmations_required;

// commutation and zero cross timers must have the same frequency
const uint32_t commutation_zc_timer_frequency = 2000000;

// remaining zero cross checks before we pass
volatile uint32_t zc_counter; // 

// open loop startup commutation timer ARR value
// TODO do this in human-readable time (microseconds) 
const uint16_t startup_commutation_period_ticks = 5000;

// commutation timer isr
void tim16_isr() {
  if (timer_get_flag(COMMUTATION_TIMER, TIM_SR_UIF)) {
    bridge_commutate();
    comparator_zc_isr_disable();

    zc_counter = zc_confirmations_required; // remove
    // TODO rotate table to get this right
    comparator_set_state(g_bridge_comm_step + 2);
    timer_clear_flag(COMMUTATION_TIMER, TIM_SR_UIF);
    debug_pins_toggle2();
  }

  // check cc1 interrupt
  // ccr1 = comparator blanking period
  if (timer_get_flag(COMMUTATION_TIMER, TIM_SR_CC1IF)) {
    zc_counter = zc_confirmations_required; // remove
    comparator_zc_isr_enable();
    timer_clear_flag(COMMUTATION_TIMER, TIM_SR_CC1IF);
  }
}

// zc timer isr
void tim6_isr() {
  // timeout waiting for zero-cross, go to open loop
  // TODO should load commutation timer default values
  if (timer_get_flag(ZC_TIMER, TIM_SR_UIF)) {
    starting = true;
    zc_counter = zc_confirmations_required; // remove
    timer_clear_flag(ZC_TIMER, TIM_SR_UIF);
    debug_pins_toggle2();
    debug_pins_toggle2();
  }
}

void comparator_zc_isr()
{
  while (zc_counter--) {
    if (!(COMP_CSR(COMP1) & COMP_CSR_OUT)) {
      zc_counter = zc_confirmations_required;
      return;
    }
    debug_pins_toggle0();
  }

  volatile uint16_t cnt = TIM_CNT(ZC_TIMER);
  // TODO does zero cause update event?
  TIM_CNT(ZC_TIMER) = 0;

  if (starting) {
    starting = false;
    debug_pins_toggle1();
  } else {
    if (cnt < TIM_ARR(COMMUTATION_TIMER)/2) {
      starting = true;
      zc_confirmations_required = startup_zc_confirmations_required;

      zc_counter = zc_confirmations_required;
      TIM_ARR(COMMUTATION_TIMER) = startup_commutation_period_ticks;
      debug_pins_toggle1();
      debug_pins_toggle1();
    } else if (cnt > 2*TIM_ARR(COMMUTATION_TIMER) - TIM_ARR(COMMUTATION_TIMER)/8){
      // we missed a cross
      // do nothing
    } else {
      if (cnt < 300) {
        if (run_zc_confirmations_required > 2) {
          run_zc_confirmations_required--;
        }
      } else if (cnt < 800) {
        if (run_zc_confirmations_required > 3) {
          run_zc_confirmations_required--;
        }
      } else if (cnt < 1500) {
        if (run_zc_confirmations_required > 5) {
          run_zc_confirmations_required--;
        }
      } else {
        if (run_zc_confirmations_required < slow_run_zc_confirmations_required) {
          run_zc_confirmations_required++;
        }
      }
      zc_confirmations_required = run_zc_confirmations_required;
      TIM_ARR(COMMUTATION_TIMER) = TIM_ARR(COMMUTATION_TIMER)/2 + cnt/2;
      // schedule commutation (+ timing advance)
      TIM_CNT(COMMUTATION_TIMER) = TIM_ARR(COMMUTATION_TIMER)/2 + TIM_ARR(COMMUTATION_TIMER)/4;
      debug_pins_toggle1();
      debug_pins_toggle1();
      debug_pins_toggle1();
    }
  }

  // blanking period
  TIM_CCR1(COMMUTATION_TIMER) = TIM_ARR(COMMUTATION_TIMER)/8;

  comparator_zc_isr_disable();
  TIM_SR(COMMUTATION_TIMER) = 0;
}

void commutation_timer_enable_interrupts()
{
  nvic_enable_irq(COMMUTATION_TIMER_IRQ);
  TIM_DIER(COMMUTATION_TIMER) |= TIM_DIER_CC1IE | TIM_DIER_UIE;
}

void zc_timer_enable_interrupts()
{
  nvic_enable_irq(COMMUTATION_TIMER_IRQ);
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
  comparator_set_state(g_bridge_comm_step);

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

// TODO move to hal
void system_clock_initialize()
{
#if defined(STM32F0)
  rcc_clock_setup_in_hsi_out_48mhz();
#elif defined(STM32F3)
  rcc_clock_setup_pll(&rcc_hsi_configs[1]);
#endif
}

int main()
{
  system_clock_initialize();

  debug_pins_initialize();

  bridge_initialize();

  // startup beep
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_AUDIO);
  bridge_set_audio_duty(0xf);
  bridge_set_audio_frequency(1000);
  for (uint32_t i = 0; i < 90000; i++) { float a = 0.6*9; }
  bridge_set_audio_frequency(1200);
  for (uint32_t i = 0; i < 90000; i++) { float a = 0.6*9; }
  bridge_set_audio_frequency(1600);
  for (uint32_t i = 0; i < 90000; i++) { float a = 0.6*9; }
  bridge_disable();
  for (int i = 0; i < 9999; i++) { float a = 0.6*9; }

  // initialize comparator
  comparator_initialize();

  // initialize motor run timers
  commutation_timer_initialize();
  zc_timer_initialize();

  // detect pwm input type
  console_write("waiting for pwm input...\r\n");
  pwm_input_initialize();
  // successive pwm input type detection checks passed
  uint8_t succeded = 0;
  // successive pwm input type detection checks needed
  const uint8_t succeded_needed = 20;
  pwm_input_type_t pwm_type_check;

  while (1) {
    gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
    // cache last pwm input type to compare to current type
    pwm_input_type_t pwm_type_last = pwm_type_check;
    // get current pwm input type
    pwm_type_check = pwm_input_detect_type();
    // check if pwm input type is valid
    if (pwm_type_check == PWM_INPUT_TYPE_NONE || pwm_type_check == PWM_INPUT_TYPE_UNKNOWN) {
      succeded = 0;
      continue;
    }
    // check if pwm input type matches the previous type
    if (pwm_type_check != pwm_type_last && succeded != 0) {
      succeded = 0;
      continue;
    }
    // have we passed enough checks?
    if (++succeded == succeded_needed) {
      break;
    }
  }

  // apply pwm input type
  pwm_input_set_type(pwm_type_check);

  while (!pwm_input_valid());

  // pwm input type valid confirmation beep
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_AUDIO);
  bridge_set_audio_duty(0xf);
  bridge_set_audio_frequency(1000);
  for (uint32_t i = 0; i < 180000; i++) { float a = 0.6*9; }
  bridge_disable();

  // wait for low throttle
  while (pwm_input_get_throttle() > 50);

  // low throttle armed beep
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_AUDIO);
  bridge_set_audio_duty(0xf);
  bridge_set_audio_frequency(1600);
  for (uint32_t i = 0; i < 180000; i++) { float a = 0.6*9; }
  bridge_disable();

  // prepare the motor for run mode (armed)
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_RUN);

  // initialize commutation and zc timers for open loop
  // motor won't move until throttle is applied
  start_motor();

  while(1) {
    gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
    //console_write_pwm_info();
    if (pwm_input_valid()) {
      bridge_set_run_duty(g_bridge_run_duty/2 + pwm_input_get_throttle()/2);
    } else {
      break;
    }
  }

  // disable the motor output
  bridge_disable();
  // stop commutation and zc timers
  stop_motor();
  while (1);
}
