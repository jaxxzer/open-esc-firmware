#include <bridge.h>
#include <comparator.h>
#include <console.h>
#include <pwm-input.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>

#define COMMUTATION_TIMER TIM15
#define COMMUTATION_TIMER_RCC RCC_TIM15
#define COMMUTATION_TIMER_IRQ NVIC_TIM15_IRQ
#define COMPARATOR_BLANKING_NS 500000

typedef enum {
    DIRECTION_NONE,
    DIRECTION_P,
    DIRECTION_N,
} direction_t;

const uint8_t zc_required = 10;
const uint8_t checks_required = 3;
volatile uint16_t checks_passed;
volatile uint16_t zc_count;
volatile uint16_t commutation_period;
const char* direction_string[3] = {
  "?",
  "POSITIVE",
  "NEGATIVE"
};

direction_t direction;

void tim15_isr() {
  if (timer_get_flag(COMMUTATION_TIMER, TIM_SR_UIF)) {

    if (zc_count >= zc_required) {
        if (checks_passed >= checks_required) {
            direction = DIRECTION_P;
        } else {
            direction = DIRECTION_N;
        }
    }

    checks_passed = 0;
    zc_count = 0;
    timer_clear_flag(COMMUTATION_TIMER, TIM_SR_UIF);
  }
}

void comparator_zc_isr()
{
  uint16_t cnt = TIM_CNT(COMMUTATION_TIMER);

  // reset timer counter
  TIM_CNT(COMMUTATION_TIMER) = 1;

  if (zc_count++ > 0) {
      if (cnt > commutation_period/4 && cnt < 4*commutation_period) {
          checks_passed++;
      }
  }

  commutation_period = cnt;
  
  // blank
  comparator_blank(COMPARATOR_BLANKING_NS);
  comparator_set_state(g_comparator_state + 1);

  gpio_toggle(LED_GPIO_PORT, LED_GPIO_PIN);
}

void commutation_timer_setup()
{
  rcc_periph_clock_enable(COMMUTATION_TIMER_RCC);

  // setup 1MHz clock, this results in 65ms period
  uint16_t psc = (rcc_ahb_frequency / 1000000) - 1;
  TIM_PSC(COMMUTATION_TIMER) = psc;
  TIM_ARR(COMMUTATION_TIMER) = 0xffff;

  // prescaler is a shadow register, and is only applied after update event
  TIM_EGR(COMMUTATION_TIMER) |= TIM_EGR_UG;

  // clear flags
  TIM_SR(COMMUTATION_TIMER) = 0;

  TIM_CR1(COMMUTATION_TIMER) |= TIM_CR1_CEN;

  // enable interrupts
  nvic_enable_irq(COMMUTATION_TIMER_IRQ);
  TIM_DIER(COMMUTATION_TIMER) |= TIM_DIER_UIE;
}

int main()
{
  console_initialize();
  console_write("\r\nWelcome to gsc: the gangster esc!\r\n");

  console_write("initializing comparator...\r\n");

  rcc_periph_clock_enable(LED_GPIO_RCC);
  gpio_mode_setup(LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GPIO_PIN);

  commutation_timer_setup();

  comparator_initialize();
  comparator_zc_isr_enable();

  while (1) {
    console_write("direction: ");
    console_write(direction_string[direction]);
    console_write("\r\n");
  }
}
