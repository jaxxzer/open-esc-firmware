// This example measures the pulse width and frequency on an input pin
// Channel 1 gives the frequency (rise -> rise time)
// Channel 2 gives the pulse width (rise -> fall time)
// the timer must have a slave controller to use this example!

#include "stm32lib-conf.h"

#define USART_BAUDRATE 115200
#define GPIO_CAPTURE_AF 2
#define CAPTURE_TIMER timer2
#define GPIO_CAPTURE_PORT GPIOA
#define GPIO_CAPTURE_PIN 15

const char* inputProtocol();
void initUsart1(void);

Gpio gpioLed{ GPIO_PORT_LED_RED, GPIO_PIN_LED_RED };

Timer &timerCapture = CAPTURE_TIMER;
Gpio gpioCapture{ GPIO_CAPTURE_PORT, GPIO_CAPTURE_PIN };
TimerChannelInput tciRising{ &timerCapture, TIM_Channel_1 };
TimerChannelInput tciFalling{ &timerCapture, TIM_Channel_2 };

int main() {
  configureClocks(1000);
  initUsart1();
#if defined(STM32F1)
  gpioLed.init(GPIO_Mode_AF_PP);
  gpioCapture.init(GPIO_Mode_IN_FLOATING);
  gpioLed.remapConfig(GPIO_LED1_REMAP, ENABLE);
  gpioCapture.remapConfig(GPIO_CAPTURE_REMAP, ENABLE);
#elif defined(STM32F0) || defined(STM32F3)
  gpioLed.init(GPIO_Mode_OUT);
  gpioCapture.init(GPIO_Mode_AF);
  gpioCapture.configAF(GPIO_CAPTURE_AF);
#else
#error
#endif

  TIM_SelectInputTrigger(timerCapture.peripheral(), TIM_TS_TI1FP1);
  TIM_SelectSlaveMode(timerCapture.peripheral(), TIM_SlaveMode_Reset);
  timerCapture.init();

  // Note CCxS bits only writable when CCxE is 0 (channel is disabled)
  tciRising.init(TIM_ICPolarity_Rising, 0x0);
  tciFalling.init(TIM_ICPolarity_Falling, 0x0, TIM_ICPSC_DIV1, TIM_ICSelection_IndirectTI);

  timerCapture.setEnabled(ENABLE);

  print_clocks();

  while (1) {
    mDelay(100);
    printf("period: %d\tduty: %d\r\n", TIM2->CCR1, TIM2->CCR2);
    printf("input protocol: %s\r\n", inputProtocol());
    gpioLed.toggle();
  }

  return 0;
}

// @72MHz...
// proshot is 290, 70~200
// dshot1200 is 60, 20/40
// dshot600 is 120, 40/80
// dshot300 is 240, 80/160
// multishot is 360~1800
// oneshot42 is 3020~6040
// oneshot125 is 8000~16000
// classical pwm is shorter 1000~2000 microseconds 2500us pulse is 180000
const char* inputProtocol()
{
  uint32_t period = timerCapture.peripheral()->CCR1;
  uint32_t duty = timerCapture.peripheral()->CCR2;
  if (period < 2000) { // high frequency/digital protocol
    // TODO use number of symbols/frame to differentiate between dshot and proshot
    if ( (duty > 65 && duty < 75) || duty > 180 ) {
      return "proshot";
    }
    return "dshot";
  } else { // analog protocol
    if (duty < 2400) {
      return "multishot";
    } else if (duty < 7000) {
      return "oneshot42";
    } else if (duty < 25000) {
      return "oneshot125";
    } else if (duty < 180000) { 
      return "classic";
    } else {
      return "noise/unknown";
    }
  }
}

void initUsart1(void) {

  Gpio gpioUsart1Tx{GPIO_USART1_TX_PORT, GPIO_USART1_TX_PIN};
  Gpio gpioUsart1Rx{GPIO_USART1_RX_PORT, GPIO_USART1_RX_PIN};

#if defined(STM32F0)
  nvic_config(USART1_IRQn, 0, ENABLE);
#elif defined(STM32F1) || defined(STM32F3)
  nvic_config(USART1_IRQn, 0, 0, ENABLE);
#else
#error
#endif

#if defined(STM32F0) || defined(STM32F3)
  gpioUsart1Rx.init(GPIO_Mode_AF, GPIO_PuPd_UP);
  gpioUsart1Tx.init(GPIO_Mode_AF, GPIO_PuPd_UP);
  gpioUsart1Rx.configAF(GPIO_USART1_AF);
  gpioUsart1Tx.configAF(GPIO_USART1_AF);
#elif defined(STM32F1)
  gpioUsart1Rx.init(GPIO_Mode_AF_PP);
  gpioUsart1Tx.init(GPIO_Mode_AF_PP);
  Gpio::remapConfig(GPIO_USART1_REMAP, ENABLE);
#else
#error
#endif

  uart1.init(USART_BAUDRATE);
  uart1.ITConfig(USART_IT_RXNE, ENABLE);
  uart1.setEnabled(ENABLE);
  uart1.cls();
}
