
extern "C" {
#include <adc.h>
#include <bridge.h>
#include <commutation.h>
#include <comparator.h>
#include <console.h>
#include <debug-pins.h>
#include <global.h>
#include <isr.h>
#include <pwm-input.h>
#include <watchdog.h>
}

// hpp
#include <io.h>

#include <libopencm3/cm3/vector.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/comparator.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

// use this throttle in open loop startup
const uint16_t startup_throttle = 75;


// declared/documented in global.h
uint16_t run_zc_confirmations_required = slow_run_zc_confirmations_required;
const uint16_t slow_run_zc_confirmations_required = 14;
volatile bool starting;
const uint16_t startup_commutation_period_ticks = 12000;
const uint16_t startup_zc_confirmations_required = 15;
volatile uint32_t zc_counter;
uint32_t zc_confirmations_required = startup_zc_confirmations_required;

// for test
void test_spin()
{
  for (uint32_t i = 0; i < 4000; i++) { io_write_state(); watchdog_reset(); }
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_RUN);
  bridge_set_run_duty(200);
  watchdog_reset();
  start_motor();
  for (uint32_t i = 200; i < 500; i++) {
    for (uint32_t j = 0; j < 100; j++) {
      watchdog_reset();
      bridge_set_run_duty(i);
      io_write_state();
    }
  }

  bridge_disable();
  stop_motor();

  while(1) {watchdog_reset();}
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
  io_write_state();

  stop_motor();
  while (1) {watchdog_reset();};
}

int main()
{
  system_clock_initialize();

  io_initialize();

  rcc_periph_clock_enable(LED_GPIO_RCC);
  gpio_mode_setup(LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GPIO_PIN);
  for (uint8_t i = 0; i < 10; i++) {
    gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
    for (uint32_t j = 0; j < 100000; j++) { float a = 0.6*9; }
  }

  debug_pins_initialize();

  console_initialize();
  console_write("welcome to open-esc!\r\n");

  for (uint32_t j = 0; j < 1000000; j++) { float a = 0.6*9; }

  adc_initialize();
  overcurrent_watchdog_initialize();
  adc_start();

  watchdog_start(10); // 10ms watchdog timeout

  bridge_initialize();

  // setup adc synchronization
  // TODO move to bridge
  nvic_enable_irq(NVIC_TIM1_CC_IRQ);
  TIM_DIER(TIM1) |= TIM_DIER_CC4IE;

  // startup beep
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_AUDIO);
  bridge_set_audio_duty(0x4);
  bridge_set_audio_frequency(1000);
  for (uint32_t i = 0; i < 40000; i++) { watchdog_reset(); io_process_input(); }
  bridge_set_audio_frequency(1200);
  for (uint32_t i = 0; i < 40000; i++) { watchdog_reset(); io_process_input(); }
  bridge_set_audio_frequency(1600);
  for (uint32_t i = 0; i < 40000; i++) { watchdog_reset(); io_process_input(); }
  bridge_disable();
  for (int i = 0; i < 9999; i++) { watchdog_reset(); io_process_input(); }

  // initialize comparator
  comparator_initialize();

  // initialize motor run timers
  commutation_timer_initialize();
  zc_timer_initialize();

  // detect pwm input type
  pwm_input_initialize();
  // successive pwm input type detection checks passed
  uint8_t succeded = 0;
  // successive pwm input type detection checks needed
  const uint8_t succeded_needed = 3;
  pwm_input_type_t pwm_type_check;

  // setup input signal timer with 25ms timeout
  // todo: rework pwm_input, throttle, and io interfaces for some sense
  pwm_input_set_type(PWM_INPUT_TYPE_CONVENTIONAL);
  while (!g.throttle_valid)
  {
    watchdog_reset();
    gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
    io_process_input();
    // todo beep once in a while
  }

  // pwm input type valid confirmation beep
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_AUDIO);
  bridge_set_audio_duty(0x4);
  bridge_set_audio_frequency(1000);
  for (uint32_t i = 0; i < 90000; i++) { watchdog_reset(); io_process_input(); }
  bridge_disable();

  // wait for low throttle
  while (g.throttle > 0) {
    watchdog_reset();
    io_process_input();
  }

  // low throttle armed beep
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_AUDIO);
  bridge_set_audio_duty(0x4);
  bridge_set_audio_frequency(1600);
  for (uint32_t i = 0; i < 90000; i++) { watchdog_reset(); io_process_input(); }
  bridge_disable();

  // prepare the motor for run mode (armed)
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_RUN);

  // we are armed

  // initialize commutation and zc timers for open loop
  // motor won't move until throttle is applied
  start_motor();

  while(1) {
    watchdog_reset();
    gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
    io_process_input();
    if (g.throttle_valid && g.throttle > startup_throttle) {
      bridge_enable();
      bridge_set_run_duty(g_bridge_run_duty/2 + g.throttle/2);
    } else {
      bridge_disable();
    }
  }

  // disable the motor output
  bridge_disable();
  // stop commutation and zc timers
  stop_motor();

  // independent watchdog will issue a system reset
  while (1) {};
}
