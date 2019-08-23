#include <bridge.h>
#include <comparator.h>
#include <console.h>
#include <pwm-input.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

void comparator_zc_isr()
{
  // blank for 1ms
  comparator_blank(1000000);
  gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
}

int main()
{
  console_initialize();
  console_write("\r\nWelcome to gsc: the gangster esc!\r\n");

  console_write("testing bridge...\r\n");

  bridge_initialize();
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_AUDIO);
  bridge_set_audio_duty(0xf);

  bridge_set_audio_frequency(800);
  for (int i = 0; i < 9999; i++) { i -= 1; float a = 0.6*9;i += 1; }
  bridge_set_audio_frequency(2000);
  for (int i = 0; i < 9999; i++) { float a = 0.6*9; }
  bridge_set_audio_frequency(3500);
  for (int i = 0; i < 9999; i++) { float a = 0.6*9; }

  bridge_set_state(BRIDGE_STATE_RUN);

  for (int i = 0; i < 12; i++) {
    bridge_commutate();
    for (int i = 0; i < 400; i++)
    {
      bridge_set_run_duty(i);
    }
  }

  bridge_disable();

  for (int i = 0; i < 9999; i++) { float a = 0.6*9; }

  console_write("initializing comparator...\r\n");

  comparator_initialize();
  comparator_zc_isr_enable();

  pwm_input_initialize();

  rcc_periph_clock_enable(LED_GPIO_RCC);

  gpio_mode_setup(LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GPIO_PIN);

  console_write("waiting for pwm input...\r\n");

  uint8_t succeded = 0;
  const uint8_t succeded_needed = 3;

  while (1) {
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
