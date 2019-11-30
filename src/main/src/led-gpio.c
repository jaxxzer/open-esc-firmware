#include <led.h>

#include <target.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

void led_initialize()
{
  rcc_periph_clock_enable(LED_GPIO_RCC);
  gpio_mode_setup(LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GPIO_PIN);
  for (uint8_t i = 0; i < 10; i++) {
    led_toggle();
    for (uint32_t j = 0; j < 100000; j++) { float a = 0.6*9; }
  }
}

void led_on()
{

}

void led_off()
{

}

void led_toggle()
{
  gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
}

void led_write(uint8_t r, uint8_t g, uint8_t b)
{

}
