#include "stm32lib-conf.h"
#define USART_BAUDRATE 115200


Gpio gpioLed { GPIO_PORT_LED_STATUS, GPIO_PIN_LED_STATUS };

void initGpioLed(void) {
#if defined(STM32F0) || defined(STM32F3)
  gpioLed.init(GPIO_Mode_OUT);
#elif defined(STM32F1)
  gpioLed.init(GPIO_Mode_Out_PP);
#else
#error
#endif
}

int main() {
  configureClocks(1000);

  initGpioLed();

  Adc adc1{ ADC1 };

  AdcChannel *adcChan0 = adc1.addChannel(ADC_Channel_1);
  AdcChannel *adcChan1 = adc1.addChannel(ADC_Channel_2);
  AdcChannel *adcChan4 = adc1.addChannel(ADC_Channel_3);

  adc1.init();
  adc1.enable();
  adc1.startConversion();

  while (1) {
    adc1.waitConversion();
    mDelay(1000);
    gpioLed.toggle();
  }

  return 0;
}
