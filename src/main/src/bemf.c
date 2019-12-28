#include <bemf.h>

#include <global.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <target.h>

#if defined(FEEDBACK_BEMF)
// stub
void bemf_initialize()
{
    rcc_periph_clock_enable(BEMF_DIVIDER_ENABLE_GPIO_RCC);
    gpio_mode_setup(BEMF_DIVIDER_ENABLE_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BEMF_DIVIDER_ENABLE_GPIO_PIN);
    gpio_set(BEMF_DIVIDER_ENABLE_GPIO, BEMF_DIVIDER_ENABLE_GPIO_PIN);
}

// stub
uint16_t bemf_get_phase_voltage(bemf_phase_e phase)
{
    return bemf_get_phase_adc_raw(phase);
}

uint16_t bemf_get_phase_adc_raw(bemf_phase_e phase)
{
    // todo move compile switch to cmake build system
    switch (phase) {
        case (BEMF_PHASE_A):
            return g.adc_buffer[ADC_CHANNEL_PHASEA_VOLTAGE];
        case (BEMF_PHASE_B):
            return g.adc_buffer[ADC_CHANNEL_PHASEB_VOLTAGE];
        case (BEMF_PHASE_C):
            return g.adc_buffer[ADC_CHANNEL_PHASEC_VOLTAGE];
        case (BEMF_PHASE_N):
        #if defined(ADC_CHANNEL_NEUTRAL_VOLTAGE)
            return g.adc_buffer[ADC_CHANNEL_NEUTRAL_VOLTAGE];
        #else
            return 0;
        #endif
    }
}
#endif
