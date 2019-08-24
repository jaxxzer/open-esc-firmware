#include <comparator.h>

#include <libopencm3/cm3/assert.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/comparator.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

volatile uint16_t comparator_states[6] = {
    COMP_CSR_HYST_LOW | 0x041,
    COMP_CSR_HYST_LOW | 0x861,
    COMP_CSR_HYST_LOW | 0x051,
    COMP_CSR_HYST_LOW | 0x841,
    COMP_CSR_HYST_LOW | 0x061,
    COMP_CSR_HYST_LOW | 0x851,
};

uint32_t comparator_blank_tick_period_ns;

// TODO move nvic isrs to target directory

// linked for f0
void adc_comp_isr() {
    comparator_zc_isr(); // user code
    exti_reset_request(EXTI21);
}

// linked for f3
void comp123_isr() {
    comparator_zc_isr();
    exti_reset_request(EXTI21);
}

void comparator_blank_complete_isr()
{
    comparator_zc_isr_enable();
}

void tim17_isr() {
    comparator_blank_complete_isr();
    timer_clear_flag(TIM17, TIM_SR_UIF);
}

void comparator_initialize()
{
    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0 | GPIO1 | GPIO4 | GPIO5);

    rcc_periph_clock_enable(RCC_SYSCFG_COMP);

    comp_select_hyst(1, COMP_CSR_HYST_MED);

    // enable comparator (there is a startup delay)
    comparator_set_state(COMP_STATE0);

    exti_set_trigger(EXTI21, EXTI_TRIGGER_BOTH);

    rcc_periph_clock_enable(COMPARATOR_BLANK_TIMER_RCC);

    nvic_enable_irq(COMPARATOR_BLANK_IRQ);
    timer_one_shot_mode(COMPARATOR_BLANK_TIMER);
    timer_enable_irq(COMPARATOR_BLANK_TIMER, TIM_DIER_UIE);

    comparator_blank_tick_period_ns = 1000000000 / (rcc_ahb_frequency / (TIM_PSC(COMPARATOR_BLANK_TIMER) + 1));
}

void comparator_set_state(comp_state_e new_state)
{
    g_comparator_state = new_state%6;
    COMP_CSR(COMP1) = comparator_states[g_comparator_state];
}

void comparator_zc_isr_enable()
{
    nvic_enable_irq(COMPARATOR_ZC_IRQ);
    exti_enable_request(EXTI21);
}

void comparator_zc_isr_disable()
{
    nvic_disable_irq(COMPARATOR_ZC_IRQ);
    exti_disable_request(EXTI21);
    exti_reset_request(EXTI21);
}

void comparator_blank(uint32_t nanoseconds)
{
    comparator_zc_isr_disable();
    timer_set_period(COMPARATOR_BLANK_TIMER, nanoseconds / comparator_blank_tick_period_ns);
    timer_enable_counter(COMPARATOR_BLANK_TIMER);
}
