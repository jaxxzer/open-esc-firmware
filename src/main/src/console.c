#include <console.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include <target.h>

#include <adc.h>
#include <usart.h>
#include <pwm-input.h>

#include <stdio.h>

void console_initialize() {
    rcc_periph_clock_enable(CONSOLE_TX_GPIO_RCC);

    gpio_mode_setup(CONSOLE_TX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, CONSOLE_TX_GPIO_PIN);

    gpio_set_af(CONSOLE_TX_GPIO_PORT, CONSOLE_TX_GPIO_AF, CONSOLE_TX_GPIO_PIN);

    rcc_periph_clock_enable(CONSOLE_RX_GPIO_RCC);

    gpio_mode_setup(CONSOLE_RX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, CONSOLE_RX_GPIO_PIN);

    gpio_set_af(CONSOLE_RX_GPIO_PORT, CONSOLE_RX_GPIO_AF, CONSOLE_RX_GPIO_PIN);

    rcc_periph_clock_enable(CONSOLE_USART_RCC);

    usart_initialize(CONSOLE_USART, 115200);
}

void console_write(const char* string)
{
   usart_write_string(CONSOLE_USART, string);
}

void console_write_int(const uint32_t i)
{
    usart_write_int(CONSOLE_USART, i);
}

const char* pwm_type_names[6] =
{
    [PWM_INPUT_TYPE_NONE] = "none",
    [PWM_INPUT_TYPE_CONVENTIONAL] = "conventional",
    [PWM_INPUT_TYPE_ONESHOT125] = "oneshot125",
    [PWM_INPUT_TYPE_ONESHOT42] = "oneshot42",
    [PWM_INPUT_TYPE_MULTISHOT] = "multishot",
    [PWM_INPUT_TYPE_UNKNOWN] = "unknown",
};

void console_write_pwm_info()
{
    char buffer[100];
    uint8_t size = snprintf(buffer, 100, "pwm type: %s\tthrottle: %d\tduty (ns): %d\tperiod (ns): %d\r\n",
        pwm_type_names[g_pwm_type],
        pwm_input_get_throttle(),
        pwm_input_get_duty_ns(),
        pwm_input_get_period_ns());
    usart_write(CONSOLE_USART, buffer, size);
}

void console_write_adc_info()
{
    char buffer[120];
    uint8_t size = snprintf(buffer, 120, "temperature: %d\tbus voltage: %d\tbus current: %d\tA: %d\tB: %d\tC: %d\tN: %d\r\n",
        adc_read_channel(ADC_IDX_TEMPERATURE),
        adc_read_channel(ADC_IDX_BUS_VOLTAGE),
        adc_read_channel(ADC_IDX_BUS_CURRENT),
        adc_read_channel(ADC_IDX_PHASEA_VOLTAGE),
        adc_read_channel(ADC_IDX_PHASEB_VOLTAGE),
        adc_read_channel(ADC_IDX_PHASEC_VOLTAGE),
        adc_read_channel(ADC_IDX_NEUTRAL_VOLTAGE));
    usart_write(CONSOLE_USART, buffer, size);
}
