#include <libopencm3/stm32/gpio.h>

#define USE_USART_2
#define GPIO_USART2_TX_PORT GPIOA
#define GPIO_USART2_TX_PIN 2
#define GPIO_USART2_RX_PORT GPIOA
#define GPIO_USART2_RX_PIN 3
#define GPIO_USART2_AF 1
#define STDOUT_USART uart2
#define GPIO_LED1_PORT GPIOA

#define GPIO_LED1_PIN GPIO5
#define GPIO_LED1_MODE GPIO_Mode_AF
#define GPIO_LED1_AF 2
#define GPIO_LED1_TIMER timer2
#define GPIO_LED1_TIM_CH TIM_Channel_1
#define USE_TIM_2
