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

#if defined(STM32F0)
 #define COMP_HYST COMP_CSR_HYST_NO
#elif defined(STM32F3)
 #define COMP_HYST COMP_CSR_HYST_NONE
#endif

volatile comp_state_e g_comparator_state;

volatile uint16_t comparator_states[6] = {
    COMP_HYST | 0x041,
    COMP_HYST | 0x851,
    COMP_HYST | 0x061,
    COMP_HYST | 0x841,
    COMP_HYST | 0x051,
    COMP_HYST | 0x861,
};

uint32_t comparator_blank_tick_period_ns;

void comparator_blank_complete_isr()
{
    comparator_zc_isr_enable();
}

void comparator_initialize()
{
    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0 | GPIO1 | GPIO4 | GPIO5);

#if defined(STM32F3)
    rcc_periph_clock_enable(RCC_SYSCFG);
#elif defined(STM32F0)
    rcc_periph_clock_enable(RCC_SYSCFG_COMP);
#else
 #error
#endif

    // enable comparator (there is a startup delay)
    comparator_set_state(COMP_STATE1);

    // setup comparator edge interrupt (zero cross)
    exti_set_trigger(EXTI21, EXTI_TRIGGER_BOTH);

    nvic_enable_irq(COMPARATOR_ZC_IRQ);

    // setup comparator blanking timer, this is not used in normal operation
    // testing purposes only
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
    exti_reset_request(EXTI21);
    exti_enable_request(EXTI21);
}

void comparator_zc_isr_disable()
{
    exti_disable_request(EXTI21);
}

void comparator_blank(uint32_t nanoseconds)
{
    comparator_zc_isr_disable();
    timer_set_period(COMPARATOR_BLANK_TIMER, nanoseconds / comparator_blank_tick_period_ns);
    timer_enable_counter(COMPARATOR_BLANK_TIMER);
}

bool comparator_get_output()
{
    return COMP_CSR(COMP1) & COMP_CSR_OUT;
}

void comparator_unblank_isr()
{
  comparator_zc_isr_enable();
}
