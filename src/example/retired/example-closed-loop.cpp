#include "stm32lib-conf.h"

// Example of closed-loop 6-step commutation
// wait 5 seconds, then the duty cycles between 3/20 ~ 5/20 power forever

typedef struct {
    uint32_t CCER; // CCER register value
    volatile uint32_t * pwmChannel; // ccr register address to write zero
    volatile uint32_t * zeroDutyChannel1; // ccr register address to write zero
    volatile uint32_t * zeroDutyChannel2; // ccr register address to write zero
    uint16_t comparatorState; // COMP register value
} commutation_state_t;

volatile commutation_state_t commutationStates[6] = {
    { 0x00000155, &TIM1->CCR1, &TIM1->CCR2, &TIM1->CCR3, 0x0041 },
    { 0x00000515, &TIM1->CCR1, &TIM1->CCR2, &TIM1->CCR3, 0x0861 },
    { 0x00000551, &TIM1->CCR2, &TIM1->CCR1, &TIM1->CCR3, 0x0061 },
    { 0x00000155, &TIM1->CCR2, &TIM1->CCR1, &TIM1->CCR3, 0x0851 },
    { 0x00000515, &TIM1->CCR3, &TIM1->CCR1, &TIM1->CCR2, 0x0051 },
    { 0x00000551, &TIM1->CCR3, &TIM1->CCR1, &TIM1->CCR2, 0x0841 },
};

static uint8_t commutationState = 0;

Timer &commutationTimer = timer15;
Timer &zeroCrossTimer = timer3;

Timer &timer = PHASE_OUTPUT_TIMER;
Gpio gpio_m1{ GPIO_PORT_HIGH_U, GPIO_PIN_HIGH_U };
Gpio gpio_m1n{ GPIO_PORT_LOW_U, GPIO_PIN_LOW_U };
Gpio gpio_m2{ GPIO_PORT_HIGH_V, GPIO_PIN_HIGH_V };
Gpio gpio_m2n{ GPIO_PORT_LOW_V, GPIO_PIN_LOW_V };
Gpio gpio_m3{ GPIO_PORT_HIGH_W, GPIO_PIN_HIGH_W };
Gpio gpio_m3n{ GPIO_PORT_LOW_W, GPIO_PIN_LOW_W };

TimerChannelOutput tco1{&timer, TIM_Channel_1};
TimerChannelOutput tco2{&timer, TIM_Channel_2};
TimerChannelOutput tco3{&timer, TIM_Channel_3};

Gpio gpioComparator0 { GPIO_PORT_BEMF, GPIO_PIN_BEMF_U };
Gpio gpioComparator1 { GPIO_PORT_BEMF, GPIO_PIN_BEMF_V };
Gpio gpioComparator2 { GPIO_PORT_BEMF, GPIO_PIN_BEMF_W };
Gpio gpioComparatorN { GPIO_PORT_BEMF, GPIO_PIN_BEMF_NEUTRAL };

Gpio gpioTest1 { GPIO_USART1_TX_PORT, GPIO_USART1_TX_PIN };
Gpio gpioTest2 { GPIO_USART1_RX_PORT, GPIO_USART1_RX_PIN };

#define COMP_MASK_OUTPUT (1 << 14)
volatile  uint8_t validCross = 0;

// schedule speed adjustment for demonstration
uint32_t nextSpeedAdjustTime = microseconds + 1000000;

// adjust the duty by this amount when 
int8_t inc = 1;

// flag to indicate we are just getting going and need another
// zero cross to get the inter-cross period measurement
bool firstcross = true;

// comparator checks required to be considered a valid zero cross
uint8_t validCrosses = 3;

// divide the inter cross measurement by this amount, then schedule the next commutation by the result
// should be 1 if zero cross is detected on every step (rising and falling edges)
// should be 2 if zero cross is detected on every other step (rising edges only)
static const uint8_t divisor = 1;

void detectZeroCross();
void commutate();
void freewheel();

extern "C" {
  void TIM15_IRQHandler(void) {
    commutate();
    TIM15->SR &= ~1;
    validCross = 0;
  }
}

// commutation timer ticks from stop
static const uint16_t baseTiming = 0xaff;
static const uint16_t minDuty = 300;
static const uint16_t maxDuty = 500;
static uint16_t duty = minDuty;


int main() {
  configureClocks(1000);

  mDelay(5000);

  // initialize gpio
  gpio_m1.init(GPIO_Mode_AF);
  gpio_m1n.init(GPIO_Mode_AF);
  gpio_m2.init(GPIO_Mode_AF);
  gpio_m2n.init(GPIO_Mode_AF);
  gpio_m3.init(GPIO_Mode_AF);
  gpio_m3n.init(GPIO_Mode_AF);

  gpio_m1.configAF(GPIO_AF_TIMER_PHASE_OUTPUT);
  gpio_m1n.configAF(GPIO_AF_TIMER_PHASE_OUTPUT);
  gpio_m2.configAF(GPIO_AF_TIMER_PHASE_OUTPUT);
  gpio_m2n.configAF(GPIO_AF_TIMER_PHASE_OUTPUT);
  gpio_m3.configAF(GPIO_AF_TIMER_PHASE_OUTPUT);
  gpio_m3n.configAF(GPIO_AF_TIMER_PHASE_OUTPUT);

  gpioComparator0.init(GPIO_Mode_AN);
  gpioComparator1.init(GPIO_Mode_AN);
  gpioComparator2.init(GPIO_Mode_AN);
  gpioComparatorN.init(GPIO_Mode_AN);

  gpioTest1.init(GPIO_Mode_OUT);
  gpioTest2.init(GPIO_Mode_OUT);

  // Initialize the comparator
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  *(volatile uint32_t *) COMP_BASE = commutationStates[commutationState].comparatorState;

  // Timer configuration
  // pwm frequency is system clock / 2000 = 72MHz/2000 = 36kHz
  timer.init(0, 2000);
  timer.setDTG(0x18); // ~333ns deadtime insertion (this is also configuring OSSR)

  // Initialize pwm channels disabled, 0 duty
  tco1.init(TIM_OCMode_PWM1, 0);
  tco2.init(TIM_OCMode_PWM1, 0);
  tco3.init(TIM_OCMode_PWM1, 0);

  // set bit 1, enable CCPC for com events
  timer.peripheral()->CR2 |= 0x1;
  timer.setEnabled(ENABLE);
  timer.setMOE(ENABLE);

  // commutate on timer update
  commutationTimer.init(29, baseTiming);
  commutationTimer.setupUpCallback(&commutate);
  commutationTimer.interruptConfig(TIM_IT_Update, ENABLE);
  nvic_config(TIM15_IRQn, 0, 0, ENABLE);
  commutationTimer.setEnabled(ENABLE);

  // 72MHz / 30 = 2.4M
  // we want 0xffff * timer period to be larger than any zero-cross interval we could expect
  // 0xffff / 2.4M = 27 milliseconds
  zeroCrossTimer.init(29, 0xffff);
  zeroCrossTimer.setEnabled(ENABLE);

  while(1) {

    detectZeroCross();

    // wait for next commutation (interrupt)
    while(validCross == validCrosses);

    // smoothly cycle duty up and down
    if (microseconds > nextSpeedAdjustTime) {
      duty += inc;
      nextSpeedAdjustTime = microseconds + 1000;
      if (duty >= maxDuty) {
        inc = -1;
      } else if (duty <= minDuty) {
        inc = 1;
      }
    }
  };

  return 0;
}

void detectZeroCross()
{
  if (commutationTimer.peripheral()->CNT > 0xff // commutation blanking, don't measure immediately after commutation
    && TIM1->CNT > 400 // pwm blanking, don't measure right after pwm on
    && ( *(volatile uint32_t *) COMP_BASE & (1<<14) ) ) // comparator state
  {
    // this qualifies as zero cross
    validCross++;
    gpioTest1.toggle();
  }

  // we have detected enough zero crosses to be sure of it, time to commutate
  if (validCross == validCrosses) {
    // zc-zc period in timer counts
    uint16_t period = zeroCrossTimer.peripheral()->CNT / divisor;
    // reset zero cross timer
    zeroCrossTimer.peripheral()->CNT = 0;

    // skip the first cross, so that we have accurately timed one period start-to-finish
    if (firstcross) {
      firstcross  = false;
      validCrosses = 4;
    } else {
      // if the zero cross period is exceptionally short, it's probably de-sync
      // start over
      if (period < commutationTimer.peripheral()->ARR / 2) {
        firstcross = true;
        validCrosses = 8;
        validCross = 0;
        
        commutationTimer.peripheral()->ARR = baseTiming;
        duty = minDuty;
      } else {
        // Schedule next commutation
        commutationTimer.peripheral()->ARR = period;
        commutationTimer.peripheral()->CNT = period/2;
        // pulse pin
        gpioTest2.toggle();
        gpioTest2.toggle();

      }
    }
  }
}

void commutate()
{
    // preload next commutation state
    commutationState++;
    commutationState %= 6;
    TIM1->CCER = commutationStates[commutationState].CCER;
    *(commutationStates[commutationState].zeroDutyChannel1) = 0;
    *(commutationStates[commutationState].zeroDutyChannel2) = 0;
    *(commutationStates[commutationState].pwmChannel) = duty;
    TIM1->EGR |= 0x20; // set bit 5 in event generation register, generate commutate event
    *(__IO uint32_t *) COMP_BASE = commutationStates[commutationState].comparatorState;

    gpioTest2.toggle();
}

void freewheel()
{
  commutationTimer.setEnabled(DISABLE);
  tco1.setCompare(0);
  tco2.setCompare(0);
  tco3.setCompare(0);
  tco1.setEnabled(ENABLE);
  tco2.setEnabled(ENABLE);
  tco3.setEnabled(ENABLE);
  tco1.setEnabledN(DISABLE);
  tco2.setEnabledN(DISABLE);
  tco3.setEnabledN(DISABLE);
  timer.setEnabled(DISABLE);
}
