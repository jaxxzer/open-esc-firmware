#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include <target.h>

#include <console.h>
#include <pwm-input.h>

int main()
{
  console_initialize();
  console_write("Welcome to gsc, the gangster esc!\r\n");

  pwm_input_initialize();

  rcc_periph_clock_enable(LED_GPIO_RCC);

  gpio_mode_setup(LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GPIO_PIN);

  while (1) {
    gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
    console_write("pwm: ");
    console_write_int(pwm_input_get_duty());
    console_write("\r\n");
  }
}
