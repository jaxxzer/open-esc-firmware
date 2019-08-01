#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include <target.h>

int main()
{
  rcc_periph_clock_enable(LED_GPIO_RCC);

  gpio_mode_setup(LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GPIO_PIN);

  while (1) {
    gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
    for (long i = 0; i < 1000000; i++) {
      asm("nop");
    }
  }
}
