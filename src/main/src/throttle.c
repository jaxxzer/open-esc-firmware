#include <throttle.h>

#include <global.h>
#include <pwm-input.h>

#include <libopencm3/stm32/timer.h>
#include <target.h>

// this is used only when pwm signal is not present
// pwm input driver manages the timeout itself
// perhaps this should be reset() or pwm_input_timer_reset()?
void throttle_timeout_reset()
{
  TIM_CNT(PWM_INPUT_TIMER) = 1;
}

void throttle_timeout_isr()
{
  g.throttle_valid = false;
}
