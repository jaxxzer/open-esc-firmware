#include <pwm-input.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

// Must be a high resolution timer!
#if PWM_INPUT_TIMER != TIM2
 #error PWM_INPUT_TIMER must be a high resolution 32 bit timer
#endif

static uint16_t psc = 0;
static uint32_t arr = 0;

void pwm_input_initialize()
{
  rcc_periph_clock_enable(PWM_INPUT_GPIO_RCC);
  gpio_mode_setup(PWM_INPUT_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_INPUT_GPIO_PIN);
  gpio_set_af(PWM_INPUT_GPIO_PORT, PWM_INPUT_GPIO_AF, PWM_INPUT_GPIO_PIN);

  // set timer prescaler so that timer counter runs at 8MHz
  psc = (rcc_ahb_frequency / 8000000) - 1;

  // arr = period / tick time
  arr = PERIOD_MAX_NS / pwm_input_tick_period_ns();

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

pwm_input_type_t pwm_input_detect_type()
{
  uint32_t duty = pwm_input_get_duty_ns();
  uint32_t period = pwm_input_get_period_ns();

  // if the signal frequency is > 32kHz, it's a digital protocol
  // if the signal period is < 1/32kHz = 31250 nanoseconds, it's a digital protocol
  if (period == 0) {
    return PWM_INPUT_TYPE_NONE;
  } else if (period < DIGITAL_PERIOD_MAX_NS) {
    return PWM_INPUT_TYPE_UNKNOWN; // no support for dshot or multishot yet
  } else {
  // otherwise, it's an 'analog' pwm protocol
  // we can determine thdutye type now by looking at the duty
    if (duty == 0) {
      return PWM_INPUT_TYPE_NONE;
    } else if (duty < MULTISHOT_DUTY_MAX_NS) {
      return PWM_INPUT_TYPE_MULTISHOT;
    } else if (duty < ONESHOT42_DUTY_MAX_NS) {
      return PWM_INPUT_TYPE_ONESHOT42;
    } else if (duty < ONESHOT125_DUTY_MAX_NS) {
      return PWM_INPUT_TYPE_ONESHOT125;
    } else if (duty < CONVENTIONAL_DUTY_MAX_NS) {
      return PWM_INPUT_TYPE_CONVENTIONAL;
    } else {
      return PWM_INPUT_TYPE_UNKNOWN;
    }
  }
}

uint32_t pwm_input_tick_period_ns()
{
  // 1e9 / clock frequency
  return 1000000000 / (rcc_ahb_frequency / (psc + 1));
}

uint32_t pwm_input_get_period()
{
  return PWM_INPUT_PERIOD_CCR(PWM_INPUT_TIMER);
}

uint32_t pwm_input_get_period_ns()
{
  // tick counts * tick period in nanoseconds
  return pwm_input_get_period() * pwm_input_tick_period_ns();
}

uint32_t pwm_input_get_duty()
{
  return PWM_INPUT_DUTY_CCR(PWM_INPUT_TIMER);
}

uint32_t pwm_input_get_duty_ns()
{
  // tick counts * tick period in nanoseconds
  return pwm_input_get_duty() * pwm_input_tick_period_ns();
}

uint16_t pwm_input_throttle_scale_ns(uint32_t duty_min_ns, uint32_t duty_max_ns)
{
  uint32_t duty = pwm_input_get_duty_ns();

  if (duty < duty_min_ns) {
    return 0;
  }

  if (duty > duty_max_ns) {
    return 0xffff;
  }

  return ((0xffff * (uint64_t)(duty - duty_min_ns))) / ((duty_max_ns - duty_min_ns));
}

uint16_t pwm_input_get_throttle()
{
  switch(g_pwm_type) {
    case PWM_INPUT_TYPE_CONVENTIONAL:
      return pwm_input_throttle_scale_ns(1000000, 2000000);
    case PWM_INPUT_TYPE_ONESHOT125:
      return pwm_input_throttle_scale_ns(125000, 250000);
    case PWM_INPUT_TYPE_ONESHOT42:
      return pwm_input_throttle_scale_ns(42000, 84000);
    case PWM_INPUT_TYPE_MULTISHOT:
      return pwm_input_throttle_scale_ns(5000, 25000);
    case PWM_INPUT_TYPE_NONE:
    case PWM_INPUT_TYPE_UNKNOWN:
    default:
      return 0;
  }
}
