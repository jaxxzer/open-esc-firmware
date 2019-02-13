#pragma once

#define USE_TIM_1
#define USE_TIM_2
#define USE_TIM_3
#define USE_TIM_6
#define USE_TIM_15
#define USE_TIM_16

#define USE_ADC_1

#define USE_COMP_1

#define ADC_CHAN_VOLTAGE_U ADC_Channel_4
#define ADC_CHAN_VOLTAGE_V ADC_Channel_5
#define ADC_CHAN_VOLTAGE_W ADC_Channel_0
#define ADC_CHAN_VOLTAGE_NEUTRAL ADC_Channel_1

#define ADC_CHAN_CURRENT_U
#define ADC_CHAN_CURRENT_V
#define ADC_CHAN_CURRENT_W

#define ADC_CHAN_VOLTAGE_BATTERY ADC_Channel_3
#define ADC_CHAN_CURRENT_BATTERY ADC_Channel_6
#define ADC_CHAN_INPUT ADC_Channel_2
#define USE_USART_1
// TODO add AF to constructor
#define GPIO_USART1_RX_PORT GPIOA
#define GPIO_USART1_RX_PIN 10
#define GPIO_USART1_TX_PORT GPIOA
#define GPIO_USART1_TX_PIN 9
#define GPIO_USART1_AF 0

// #define GPIO_UART2_RX_PORT GPIOB
// #define GPIO_UART2_RX_PIN 7
// #define GPIO_UART2_TX_PORT GPIOB
// #define GPIO_UART2_TX_PIN 6
// #define GPIO_UART2_AF 0

#define GPIO_USART2_TX_PORT GPIOA
#define GPIO_USART2_TX_PIN 2
#define GPIO_USART2_RX_PORT GPIOA
#define GPIO_USART2_RX_PIN 3
#define GPIO_USART2_AF 1
#define STDOUT_USART uart2
#define USE_USART_2

#define GPIO_LED_RED 0xA1
#define GPIO_LED_BLUE { GPIOB, 3 }
#define GPIO_LED_GREEN { GPIOB, 4 }

#define GPIO_HI_U { GPIOA, 8 }
#define GPIO_HI_V { GPIOA, 9 }
#define GPIO_HI_W { GPIOA, 10 }

#define GPIO_LO_U { GPIOA, 7 }
#define GPIO_LO_V { GPIOB, 0 }
#define GPIO_LO_W { GPIOB, 1 }

#define GPIO_HALL_1 { GPIOB, 5 }





#define GPIO_LED_EXTERNAL_PORT      GPIOB
#define GPIO_LED_EXTERNAL_PIN       1

#define GPIO_LED_INTERNAL_PORT      GPIOA
#define GPIO_LED_INTERNAL_PIN       2

#define GPIO_MOSFET_GATE_PORT  GPIOA
#define GPIO_MOSFET_GATE_PIN   4

#define GPIO_DIP_0_PORT        GPIOA
#define GPIO_DIP_0_PIN         5

#define GPIO_DIP_1_PORT        GPIOA
#define GPIO_DIP_1_PIN         6

#define ADC_CH_VOLTAGE     ADC_Channel_0
#define ADC_CH_TEMPERATURE ADC_Channel_1
#define ADC_CH_CURRENT     ADC_Channel_3
