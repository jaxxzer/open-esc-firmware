#include "stm32lib-conf.h"

typedef struct {
    uint32_t CCER; // which channel/nchannels to activate for this step
    volatile uint32_t * pwmChannel; // ccr register address to write duty cycle
    volatile uint32_t * zeroDutyChannel1; // ccr register address to write zero
    volatile uint32_t * zeroDutyChannel2; // ccr register address to write zero
} commutation_state_t;

volatile commutation_state_t commutationStates[6] = {
    { 0x00000155, &TIM1->CCR1, &TIM1->CCR2, &TIM1->CCR3 }, // set CCER bits  8, 6, 4, 2, 0
    { 0x00000515, &TIM1->CCR1, &TIM1->CCR2, &TIM1->CCR3 }, // set CCER bits 10, 8, 4, 2, 0
    { 0x00000551, &TIM1->CCR2, &TIM1->CCR1, &TIM1->CCR3 }, // set CCER bits 10, 8, 6, 4, 0
    { 0x00000155, &TIM1->CCR2, &TIM1->CCR1, &TIM1->CCR3 }, // set CCER bits  8, 6, 4, 2, 0
    { 0x00000515, &TIM1->CCR3, &TIM1->CCR1, &TIM1->CCR2 }, // set CCER bits 10, 8, 4, 2, 0
    { 0x00000551, &TIM1->CCR3, &TIM1->CCR1, &TIM1->CCR2 }, // set CCER bits 10, 8, 6, 4, 0
};

static uint8_t commutationState = 0;
static uint16_t duty = 100;

// TODO OCxM bits

//            -----------------------------------------------
//           | Step0 | Step1 | Step2 | Step3 | Step4 | Step5 |
//----------------------------------------------------------
//|Channel1  |   1   |   1   |   1   |   1   |   1   |   1   |
//----------------------------------------------------------
//|Channel1N |   1   |   1   |   0   |   1   |   1   |   0   |
//----------------------------------------------------------
//|Duty      |   ?   |   ?   |   0   |   0   |   0   |   0   |
//----------------------------------------------------------
//|Channel2  |   1   |   1   |   1   |   1   |   1   |   1   |
//----------------------------------------------------------
//|Channel2N |   1   |   0   |   1   |   1   |   0   |   1   |
//----------------------------------------------------------
//|Duty      |   0   |   0   |   ?   |   ?   |   0   |   0   |
//----------------------------------------------------------
//|Channel3  |   1   |   1   |   1   |   1   |   1   |   1   |
//----------------------------------------------------------
//|Channel3N |   0   |   1   |   1   |   0   |   1   |   1   |
//----------------------------------------------------------
//|Duty      |   0   |   0   |   0   |   0   |   ?   |   ?   |

//  x = ZC    -----------------------------------------------
//           | Step0 | Step1 | Step2 | Step3 | Step4 | Step5 |
//-----------------------------------------------------------
//|Channel1  |‾|_|‾|_|‾|_|‾|_________________________________|
//-------------------------------x-----------------------x---
//|Channel1N |_|‾|_|‾|_|‾|_|‾|_______|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|_______|
//-----------------------------------------------------------
//|Channel2  |_______________|‾|_|‾|_|‾|_|‾|_________________|
//-----------------------x-----------------------x------------
//|Channel2N |‾‾‾‾‾‾‾|_________|‾|_|‾|_|‾|_|‾|_______|‾‾‾‾‾‾‾|
//-----------------------------------------------------------
//|Channel3  |_______________________________|‾|_|‾|_|‾|_|‾|_|
//---------------x-----------------------x-------------------
//|Channel3N |_______|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|_________|‾|_|‾|_|‾|_|‾|

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

void commutate() {
  // increment and wrap commutation step
  commutationState++;
  commutationState %= 6;

  TIM1->CCER = commutationStates[commutationState].CCER;
  *(commutationStates[commutationState].zeroDutyChannel1) = 0;
  *(commutationStates[commutationState].zeroDutyChannel2) = 0;
  *(commutationStates[commutationState].pwmChannel) = 47;

  // set bit 5 in event generation register, generate commutate event
  // TODO move to top of call, and preload next commutation state after generating the event
  TIM1->EGR |= 0x20;
}

int main() {
  configureClocks(1000);

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

  // Timer configuration
  timer.init(0, 2000);
  timer.setDTG(0x18); // ~333ns deadtime insertion (this also sets ossr)

  tco1.init(TIM_OCMode_PWM1, 0);
  tco2.init(TIM_OCMode_PWM1, 0);
  tco3.init(TIM_OCMode_PWM1, 0);

  // set bit 1, enable CCPC for com events
  TIM1->CR2 |= 0x1;
  timer.setEnabled(ENABLE);
  timer.setMOE(ENABLE);

  while(1) {
      mDelay(10);
      commutate();
  };

  return 0;
}
