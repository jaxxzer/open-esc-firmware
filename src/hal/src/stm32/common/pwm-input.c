#include <pwm-input.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

static uint16_t psc = 0;
static uint16_t arr = 0xffff;

void pwm_input_initialize()
{
  rcc_periph_clock_enable(PWM_INPUT_GPIO_RCC);
  gpio_mode_setup(PWM_INPUT_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_INPUT_GPIO_PIN);
  gpio_set_af(PWM_INPUT_GPIO_PORT, PWM_INPUT_GPIO_AF, PWM_INPUT_GPIO_PIN);

  // set timer prescaler so that timer counter runs at 1MHz
  // the pwm duty counter then measures pulse length in microseconds
  psc = (rcc_ahb_frequency / 1000000) - 1;

  // trigger input 1 will:
  // 1. capture the signal period in CCR1 (rising edge - rising edge time)
  // 2. reset the counter (the timer keeps running)
  rcc_periph_clock_enable(PWM_INPUT_TIMER_RCC); // enable timer clock
  // stretch clock with larger dividers in order to time longer signals without overruns
  timer_set_prescaler(PWM_INPUT_TIMER, psc);
  timer_set_period(PWM_INPUT_TIMER, arr);                                     // set ARR
  timer_ic_set_input(PWM_INPUT_TIMER, PWM_INPUT_TIMER_IC_ID_RISE, PWM_INPUT_TIMER_IC_TRIGGER); // set both input channels to trigger input 1
  timer_ic_set_input(PWM_INPUT_TIMER, PWM_INPUT_TIMER_IC_ID_FALL, PWM_INPUT_TIMER_IC_TRIGGER);
  // set second input channel trigger polarity to falling (rising is default, first input channel is rising)
  // input/output configurations are on the same register (so we use set oc_polarity..)
  timer_set_oc_polarity_low(PWM_INPUT_TIMER, PWM_INPUT_TIMER_OC_ID_FALL);

  timer_ic_enable(PWM_INPUT_TIMER, PWM_INPUT_TIMER_IC_ID_RISE);
  timer_ic_enable(PWM_INPUT_TIMER, PWM_INPUT_TIMER_IC_ID_FALL);

  // set slave mode, reset counter on trigger
  timer_slave_set_mode(PWM_INPUT_TIMER, PWM_INPUT_TIMER_SLAVE_MODE);

  // set slave mode trigger to trigger input 1
  timer_slave_set_trigger(PWM_INPUT_TIMER, PWM_INPUT_TIMER_SLAVE_TRIGGER);

  timer_enable_counter(PWM_INPUT_TIMER); // set CEN
}

uint32_t pwm_input_get_duty()
{
  return PWM_INPUT_DUTY_CCR(PWM_INPUT_TIMER);
}
