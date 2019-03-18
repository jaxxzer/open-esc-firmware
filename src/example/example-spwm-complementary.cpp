#include "../sine-lookup.h"
#include "stm32lib-conf.h"
#include <math.h>
#include "arm_math.h"

// open loop sine wave modulated pwm (3 phase ac)
// duty and resolution/speed are set by genlookup.py (sine-lookup.h)

#define RESOLUTION 300
#define RESOLUTION1 100
#define RESOLUTION2 200

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

uint16_t _field_angle = 0;

void setFieldAngle(float angle);

// rotate the field by 360/resolution
// to slow down, increase the resolution
void updateSPWM(void);

int main() {
  configureClocks(1000);

// Thank you so much:
// https://www.silabs.com/community/mcu/32-bit/knowledge-base.entry.html/2014/04/16/how_to_enable_hardwa-vM9u
// Set floating point coprosessor access mode. */ 
  SCB->CPACR |= ((3UL << 10*2) | /* set CP10 Full Access */ 
  3UL << 11*2) ); /* set CP11 Full Access */ 

  mDelay(2000);

#if defined(STM32F0)
#error
#elif defined(STM32F3) // should work with f0 too
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
#else
#error
#endif

  timer.init(0, 1000); // 48kHz pwm frequency
  timer.setDTG(0x18); // ~333ns deadtime insertion

  tco1.init(TIM_OCMode_PWM1, 0, TIM_OutputState_Enable, TIM_OutputNState_Enable);
  tco2.init(TIM_OCMode_PWM1, 0, TIM_OutputState_Enable, TIM_OutputNState_Enable);
  tco3.init(TIM_OCMode_PWM1, 0, TIM_OutputState_Enable, TIM_OutputNState_Enable);

  timer.setEnabled(ENABLE);

  timer.setMOE(ENABLE);

  while (microseconds < 5000000) {
    mDelay(1);
    updateSPWM();
  }

  tco1.setEnabledN(DISABLE);
  tco2.setEnabledN(DISABLE);
  tco3.setEnabledN(DISABLE);
  timer.setEnabled(DISABLE);

  return 0;
}

void setFieldAngle(float angle) {
  while (angle > 2 * M_PI) {
    angle -= 2 * M_PI;
    if (angle < 0)
      angle = 0;
  }

  _field_angle = angle;
}

void updateSPWM(void) {
  _field_angle += 5; // to speed up, increase the increment
  _field_angle = _field_angle % RESOLUTION;
  tco1.setDuty(lookup[(_field_angle + RESOLUTION1) % RESOLUTION]);
  tco2.setDuty(lookup[_field_angle]);
  tco3.setDuty(lookup[(_field_angle + RESOLUTION2) % RESOLUTION]);
}
