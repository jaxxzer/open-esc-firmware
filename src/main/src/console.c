#include <console.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include <target.h>

#include <usart.h>
#include <pwm-input.h>

void console_initialize() {
    rcc_periph_clock_enable(CONSOLE_TX_GPIO_RCC);

    gpio_mode_setup(CONSOLE_TX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, CONSOLE_TX_GPIO_PIN);

    gpio_set_af(CONSOLE_TX_GPIO_PORT, CONSOLE_TX_GPIO_AF, CONSOLE_TX_GPIO_PIN);

    rcc_periph_clock_enable(CONSOLE_USART_RCC);

    // higher clock speed setup necessary for faster baudrates
    usart_initialize(CONSOLE_USART, 9600);
}

void console_write(const char* string)
{
   usart_write(CONSOLE_USART, string);
}

void console_write_int(const uint32_t i)
{
    usart_write_int(CONSOLE_USART, i);
}

void console_write_pwm_info()
{
    console_write_int(g_pwm_type);
    console_write("\tthrottle: ");
    console_write_int(pwm_input_get_throttle());
    console_write("\tduty: ");
    console_write_int(pwm_input_get_duty_ns());
    console_write("\tperiod: ");
    console_write_int(pwm_input_get_period_ns());
    console_write("\r\n");
}
