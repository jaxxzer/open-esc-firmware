#include <debug-pins.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include <target.h>

#define DEBUG_PINS

void debug_pins_initialize()
{
#if defined(DEBUG_PINS)
  rcc_periph_clock_enable(DBG0_GPIO_RCC);
  rcc_periph_clock_enable(DBG1_GPIO_RCC);
  rcc_periph_clock_enable(DBG2_GPIO_RCC);
  for (uint32_t i = 0; i < 10000; i++) { float a = 0.6*9; }

  gpio_mode_setup(DBG0_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DBG0_GPIO_PIN);
  gpio_mode_setup(DBG1_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DBG1_GPIO_PIN);
  gpio_mode_setup(DBG2_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DBG2_GPIO_PIN);
  for (uint32_t i = 0; i < 10000; i++) { float a = 0.6*9; }

  debug_pins_toggle0();
  debug_pins_toggle0();
  for (uint32_t i = 0; i < 10000; i++) { float a = 0.6*9; }

  debug_pins_toggle1();
  debug_pins_toggle1();
  debug_pins_toggle1();
  debug_pins_toggle1();
  for (uint32_t i = 0; i < 10000; i++) { float a = 0.6*9; }

  debug_pins_toggle2();
  debug_pins_toggle2();
  debug_pins_toggle2();
  debug_pins_toggle2();
  debug_pins_toggle2();
  debug_pins_toggle2();
  for (uint32_t i = 0; i < 90000; i++) { float a = 0.6*9; }
#endif
}

void debug_pins_toggle0()
{
#if defined(DEBUG_PINS)
  gpio_toggle(DBG0_GPIO_PORT, DBG0_GPIO_PIN);
#endif
}

void debug_pins_toggle1()
{
#if defined(DEBUG_PINS)
  gpio_toggle(DBG1_GPIO_PORT, DBG1_GPIO_PIN);
#endif
}

void debug_pins_toggle2()
{
#if defined(DEBUG_PINS)
  gpio_toggle(DBG2_GPIO_PORT, DBG2_GPIO_PIN);
#endif
}
