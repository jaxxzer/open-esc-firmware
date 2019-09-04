#include <bridge.h>

#include <libopencm3/cm3/assert.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

typedef struct {
    uint32_t CCER; // CCER register value
    volatile uint32_t* pwmChannel; // ccr register address to write zero
    volatile uint32_t* zeroDutyChannel1; // ccr register address to write zero
    volatile uint32_t* zeroDutyChannel2; // ccr register address to write zero
} commutation_state_t;

#define TIM1_CCR1_ADDR (volatile uint32_t *)(TIM1 + 0x34)
#define TIM1_CCR2_ADDR (volatile uint32_t *)(TIM1 + 0x38)
#define TIM1_CCR3_ADDR (volatile uint32_t *)(TIM1 + 0x3C)

volatile commutation_state_t bridge_comm_states[6] = {
    { 0x00000155, TIM1_CCR1_ADDR, TIM1_CCR2_ADDR, TIM1_CCR3_ADDR },
    { 0x00000515, TIM1_CCR1_ADDR, TIM1_CCR2_ADDR, TIM1_CCR3_ADDR },
    { 0x00000551, TIM1_CCR2_ADDR, TIM1_CCR1_ADDR, TIM1_CCR3_ADDR },
    { 0x00000155, TIM1_CCR2_ADDR, TIM1_CCR1_ADDR, TIM1_CCR3_ADDR },
    { 0x00000515, TIM1_CCR3_ADDR, TIM1_CCR1_ADDR, TIM1_CCR2_ADDR },
    { 0x00000551, TIM1_CCR3_ADDR, TIM1_CCR1_ADDR, TIM1_CCR2_ADDR },
};

void bridge_gpio_setup(
    enum rcc_periph_clken rcc,
    uint32_t port,
    uint16_t pin,
    uint8_t af)
{
    rcc_periph_clock_enable(rcc);
    gpio_mode_setup(port, GPIO_MODE_AF, GPIO_PUPD_NONE, pin);
    gpio_set_af(port, af, pin);
}

void bridge_initialize()
{
    // TODO optimize
    bridge_gpio_setup(
        BRIDGE_HI_A_GPIO_RCC,
        BRIDGE_HI_A_GPIO_PORT,
        BRIDGE_HI_A_GPIO_PIN,
        BRIDGE_GPIO_AF);

    bridge_gpio_setup(
        BRIDGE_HI_B_GPIO_RCC,
        BRIDGE_HI_B_GPIO_PORT,
        BRIDGE_HI_B_GPIO_PIN,
        BRIDGE_GPIO_AF);

    bridge_gpio_setup(
        BRIDGE_HI_C_GPIO_RCC,
        BRIDGE_HI_C_GPIO_PORT,
        BRIDGE_HI_C_GPIO_PIN,
        BRIDGE_GPIO_AF);

    bridge_gpio_setup(
        BRIDGE_LO_A_GPIO_RCC,
        BRIDGE_LO_A_GPIO_PORT,
        BRIDGE_LO_A_GPIO_PIN,
        BRIDGE_GPIO_AF);

    bridge_gpio_setup(
        BRIDGE_LO_B_GPIO_RCC,
        BRIDGE_LO_B_GPIO_PORT,
        BRIDGE_LO_B_GPIO_PIN,
        BRIDGE_GPIO_AF);

    bridge_gpio_setup(
        BRIDGE_LO_C_GPIO_RCC,
        BRIDGE_LO_C_GPIO_PORT,
        BRIDGE_LO_C_GPIO_PIN,
        BRIDGE_GPIO_AF);

    // initialize timer
    rcc_periph_clock_enable(RCC_TIM1);
    
    timer_set_prescaler(TIM1, 0);
    timer_set_period(TIM1, 2048);
    timer_set_oc_value(TIM1, TIM_OC1, 0);
    timer_set_oc_value(TIM1, TIM_OC2, 0);
    timer_set_oc_value(TIM1, TIM_OC3, 0);
    timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);
    timer_set_oc_mode(TIM1, TIM_OC2, TIM_OCM_PWM1);
    timer_set_oc_mode(TIM1, TIM_OC3, TIM_OCM_PWM1);
    timer_enable_oc_preload(TIM1, TIM_OC1);
    timer_enable_oc_preload(TIM1, TIM_OC2);
    timer_enable_oc_preload(TIM1, TIM_OC3);
    timer_enable_oc_output(TIM1, TIM_OC1);
    timer_enable_oc_output(TIM1, TIM_OC1N);
    timer_enable_oc_output(TIM1, TIM_OC2);
    timer_enable_oc_output(TIM1, TIM_OC2N);
    timer_enable_oc_output(TIM1, TIM_OC3);
    timer_enable_oc_output(TIM1, TIM_OC3N);

    timer_set_deadtime(TIM1, 0x40);
    
    timer_set_enabled_off_state_in_idle_mode(TIM1); // get this bit wrong and blow your bridge
    timer_set_enabled_off_state_in_run_mode(TIM1);

    timer_enable_counter(TIM1);

    // TODO lock registers
}

void bridge_set_state(bridge_state_e new_state)
{
    cm3_assert(g_bridge_state != new_state);

    switch (new_state) {
    case BRIDGE_STATE_DISABLED:
        bridge_disable();
        break;
    case BRIDGE_STATE_AUDIO:
        bridge_set_audio_duty(0);
        TIM1_ARR = 127;
        g_bridge_state = BRIDGE_STATE_AUDIO;
        break;
    case BRIDGE_STATE_RUN:
        bridge_set_run_duty(0);
        TIM1_PSC = 0;
        TIM1_ARR = 2047;
        g_bridge_state = BRIDGE_STATE_RUN;
        break;
    default:
        cm3_assert_not_reached();
        break;
    }
}

void bridge_enable() {
    cm3_assert(g_bridge_state == BRIDGE_STATE_DISABLED);
    timer_enable_break_main_output(TIM1); // shouldn't do/be part of f0 aapi?
}

void bridge_disable() {
    timer_disable_break_main_output(TIM1); // shouldn't do/be part of f0 aapi?
    bridge_set_run_duty(0);
    bridge_set_audio_duty(0);
    g_bridge_state = BRIDGE_STATE_DISABLED;
}

// for use in audio mode - duty is always 4 bit (0-16)
// duty may never exceed 12.5%
void bridge_set_audio_duty(uint8_t duty) {
    cm3_assert(g_bridge_state == BRIDGE_STATE_AUDIO);

    g_bridge_audio_duty = duty & 0xf;
    TIM1_CCR1 = g_bridge_audio_duty;
}

void bridge_set_audio_frequency(uint16_t frequency) {
    cm3_assert(g_bridge_state == BRIDGE_STATE_AUDIO);

    // frequency = ahb clock / prescaler / arr
    // psc = ahb clock / arr / frequency
    uint16_t psc = rcc_ahb_frequency / TIM1_ARR / frequency;
    TIM1_PSC = psc;
}

// for use in run mode - duty is always 12bit (0-2047)
void bridge_set_run_duty(uint16_t duty)
{
    cm3_assert(g_bridge_state == BRIDGE_STATE_RUN);

    if (duty > 512) {
        duty = 512; // clamp duty to ~25% max for testing
    }

    g_bridge_run_duty = duty;
    // *(bridge_comm_states[g_bridge_comm_step].pwmChannel) = g_bridge_run_duty;
}

void bridge_commutate() {
    cm3_assert(state == BRIDGE_STATE_RUN);

    TIM_EGR(TIM1) |= 0x20; // set bit 5 in event generation register, generate commutate event

    // preload next commutation state
    g_bridge_comm_step++;
    g_bridge_comm_step %= 6;

    *(bridge_comm_states[g_bridge_comm_step].zeroDutyChannel1) = 0;
    *(bridge_comm_states[g_bridge_comm_step].zeroDutyChannel2) = 0;
    *(bridge_comm_states[g_bridge_comm_step].pwmChannel) = g_bridge_run_duty;
    TIM_CCER(TIM1) = bridge_comm_states[g_bridge_comm_step].CCER;
}
