
extern "C" {
#include <adc.h>
#include <bridge.h>
#include <comparator.h>
#include <console.h>
#include <debug-pins.h>
#include <global.h>
#include <isr.h>
#include <pwm-input.h>
#include <throttle.h>
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

// if true, we are in open-loop
// we wait for the first zero cross period (2 sequential valid zero crosses)
// timing, then we will enter closed loop
volatile bool starting;

// use this throttle in open loop startup
const uint16_t startup_throttle = 75;

// the comparator output must hold for this many checks in a row before we consider it a valid zero-cross
// this can be descreased as rpm increases
const uint16_t startup_zc_confirmations_required = 15;
const uint16_t slow_run_zc_confirmations_required = 14;

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
const uint16_t startup_commutation_period_ticks = 12000;

void commutation_isr()
{
    bridge_commutate();
    comparator_zc_isr_disable();

    zc_counter = zc_confirmations_required; // remove
    // TODO rotate table to get this right
    comparator_set_state((comp_state_e)(g_bridge_comm_step + 2));
    debug_pins_toggle2();
}

void comparator_unblank_isr()
{
  zc_counter = zc_confirmations_required; // remove
  comparator_zc_isr_enable();
}

// zc timer isr
void comparator_zc_timeout_isr() {
  // timeout waiting for zero-cross, go to open loop
  // TODO should load commutation timer default values
  starting = true;
  zc_counter = zc_confirmations_required; // remove
  debug_pins_toggle2();
  debug_pins_toggle2();
}

void comparator_zc_isr()
{
  while (zc_counter--) {
    if (!comparator_get_output()) {
      zc_counter = zc_confirmations_required;
      comparator_blank(20000); // give main loop a gasp of air
      return;
    }
    debug_pins_toggle0();
  }

  volatile uint16_t cnt = TIM_CNT(ZC_TIMER);
  // TODO does zero cause update event?
  TIM_CNT(ZC_TIMER) = 0;


  if (cnt < 300) {
    starting = true;
  }
  if (cnt < TIM_ARR(COMMUTATION_TIMER)/2) {
    if (starting) {
      zc_confirmations_required = startup_zc_confirmations_required;

      zc_counter = zc_confirmations_required;
      TIM_ARR(COMMUTATION_TIMER) = startup_commutation_period_ticks;
      debug_pins_toggle1();
      debug_pins_toggle1();
    }
  } else if (cnt > TIM_ARR(COMMUTATION_TIMER) + TIM_ARR(COMMUTATION_TIMER)/2 + TIM_ARR(COMMUTATION_TIMER)/4) {
    // we missed a cross
    // do nothing
  } else {
    if (starting) {
      starting = false;
      debug_pins_toggle1();
    } else {
      if (cnt < 6000) {
        if (run_zc_confirmations_required > 6) {
          run_zc_confirmations_required--;
        }
      } else if (cnt < 8000) {
        if (run_zc_confirmations_required > 8) {
          run_zc_confirmations_required--;
        }
      } else if (cnt < 10000) {
        if (run_zc_confirmations_required > 10) {
          run_zc_confirmations_required--;
        }
      } else {
        if (run_zc_confirmations_required < slow_run_zc_confirmations_required) {
          run_zc_confirmations_required++;
        }
      }
      zc_confirmations_required = run_zc_confirmations_required;
      TIM_ARR(COMMUTATION_TIMER) = TIM_ARR(COMMUTATION_TIMER)/2 + TIM_ARR(COMMUTATION_TIMER)/4 + cnt/4;
      // TIM_ARR(COMMUTATION_TIMER) = TIM_ARR(COMMUTATION_TIMER)/2 + cnt/2;
      // schedule commutation (+ timing advance)
      TIM_CNT(COMMUTATION_TIMER) = TIM_ARR(COMMUTATION_TIMER)/2;
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
