#include <comparator.h>

#include <libopencm3/cm3/assert.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

volatile comp_state_e g_comparator_state;

typedef struct
{
    uint32_t port; // gpio port
    uint16_t pin; // gpio pin
    uint32_t exti; // exti line
    enum exti_trigger_type trigger; // exti trigger edge (rising or falling)
    uint8_t nvic; // nvic irq

} comparator_state_t;

volatile comparator_state_t comparator_states[6] = {
    GPIOF, GPIO1, EXTI1, EXTI_TRIGGER_FALLING, NVIC_EXTI0_1_IRQ,
    GPIOB, GPIO1, EXTI1, EXTI_TRIGGER_RISING, NVIC_EXTI0_1_IRQ,
    GPIOF, GPIO0, EXTI0, EXTI_TRIGGER_FALLING, NVIC_EXTI0_1_IRQ,
    GPIOF, GPIO1, EXTI1, EXTI_TRIGGER_RISING, NVIC_EXTI0_1_IRQ,
    GPIOB, GPIO1, EXTI1, EXTI_TRIGGER_FALLING, NVIC_EXTI0_1_IRQ,
    GPIOF, GPIO0, EXTI0, EXTI_TRIGGER_RISING, NVIC_EXTI0_1_IRQ,
};

uint32_t comparator_blank_tick_period_ns;

// TODO move nvic isrs to target directory

// linked for f0
void exti0_1_isr() {
    comparator_zc_isr(); // user code
    exti_reset_request(comparator_states[g_comparator_state].exti);
}

void comparator_blank_complete_isr()
{
    comparator_zc_isr_enable();
}

void comparator_initialize()
{
#if defined(STM32F3)
    rcc_periph_clock_enable(RCC_SYSCFG);
#elif defined(STM32F0)
    rcc_periph_clock_enable(RCC_SYSCFG_COMP);
#else
# error
#endif

    rcc_periph_clock_enable(COMPARATOR_A_GPIO_RCC);
    rcc_periph_clock_enable(COMPARATOR_B_GPIO_RCC);
    rcc_periph_clock_enable(COMPARATOR_C_GPIO_RCC);

    // gpio_mode_setup(COMPARATOR_A_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, COMPARATOR_A_GPIO_PIN);
    // gpio_mode_setup(COMPARATOR_B_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, COMPARATOR_B_GPIO_PIN);
    // gpio_mode_setup(COMPARATOR_C_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, COMPARATOR_C_GPIO_PIN);
    gpio_mode_setup(COMPARATOR_A_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, COMPARATOR_A_GPIO_PIN);
    gpio_mode_setup(COMPARATOR_B_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, COMPARATOR_B_GPIO_PIN);
    gpio_mode_setup(COMPARATOR_C_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, COMPARATOR_C_GPIO_PIN);

    // enable comparator (there is a startup delay)
    comparator_set_state(COMP_STATE0);

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
    // de-initialize current state
    //comparator_zc_isr_disable();

    g_comparator_state = new_state%6;
    exti_select_source(comparator_states[g_comparator_state].exti, comparator_states[g_comparator_state].port);
    exti_set_trigger(comparator_states[g_comparator_state].exti, comparator_states[g_comparator_state].trigger);
}

void comparator_zc_isr_enable()
{
    exti_reset_request(comparator_states[g_comparator_state].exti);
    exti_enable_request(comparator_states[g_comparator_state].exti);
    nvic_enable_irq(comparator_states[g_comparator_state].nvic);
}

void comparator_zc_isr_disable()
{
    nvic_disable_irq(comparator_states[g_comparator_state].nvic);
    nvic_clear_pending_irq(comparator_states[g_comparator_state].nvic);
    exti_disable_request(comparator_states[g_comparator_state].exti);
    exti_reset_request(comparator_states[g_comparator_state].exti);
}

void comparator_blank(uint32_t nanoseconds)
{
    comparator_zc_isr_disable();
    timer_set_period(COMPARATOR_BLANK_TIMER, nanoseconds / comparator_blank_tick_period_ns);
    timer_enable_counter(COMPARATOR_BLANK_TIMER);
}

bool comparator_get_output()
{
    if (comparator_states[g_comparator_state].trigger == EXTI_TRIGGER_RISING) {
        return gpio_get(comparator_states[g_comparator_state].port, comparator_states[g_comparator_state].pin);
    } else {
        return !gpio_get(comparator_states[g_comparator_state].port, comparator_states[g_comparator_state].pin);
    }
}

void comparator_unblank_isr()
{
    comparator_zc_isr_enable();
}
