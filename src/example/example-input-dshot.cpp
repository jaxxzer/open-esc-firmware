// dshot decoding
// this is capable of decoding dshot frames at at least 8kHz with minimal cpu overhead
// cc3 interrupt fires at the end of a packet
// this code autobauds, it does not discriminate between dshot300, 600, 1200 etc.
// the single limiting factor is DSHOT timeout counts, increase this number to increase the MINIMUM acceptable dshot bitrate
// the maximum dshot bitrate is determined by the timer clock frequency, this might be equivalent to
// ~dshot5000 with a 72MHz clock in tame (low noise) electrical environments
// the default of 400 is fitting for dshot 300 and frame frequencies (1/looptimes) of up to 8kHz

#include "stm32lib-conf.h"

#define DSHOT_TIMEOUT_COUNTS 400
#define USART_BAUDRATE 115200
#define GPIO_CAPTURE_AF 2
#define CAPTURE_TIMER timer2
#define GPIO_CAPTURE_PORT GPIOA
#define GPIO_CAPTURE_PIN 15
#define TIMER_DMA_CHANNEL DMA1_Channel3

Dma dmaChannel = Dma( TIMER_DMA_CHANNEL );

Gpio gpioLed { GPIO_LED_RED_PORT, GPIO_LED_RED_PIN };

Timer &timerCapture = CAPTURE_TIMER;
Gpio gpioCapture{ GPIO_CAPTURE_PORT, GPIO_CAPTURE_PIN };
TimerChannelInput tciRising{&timerCapture, TIM_Channel_1};
TimerChannelInput tciFalling{&timerCapture, TIM_Channel_2};
TimerChannelOutput tcoFraming{&timerCapture, TIM_Channel_3};

const uint8_t maxFrameLength = 50;
volatile uint16_t fallCaptures[maxFrameLength];
volatile uint8_t frameLength = 0; // number of bits in last dshot frame received
volatile uint16_t dshotThreshold = 0;
volatile bool gotCapture = false;
volatile uint32_t decodedPackets = 0;

// Triggered on start of frame
void cc2Callback(void);

// triggered on timeout waiting for next bit in frame (end of frame)
void cc3Callback(void);

void initUsart1(void);

// ch1 -> rising counter is captured into ccr1 on rising edges tells you the frequency of pulse (ccr == time since last
// rising edge) ch2 -> falling timer counter is captured into ccr2 on falling edges ch3 -> reset and trigger timer on
// ccr match (make this less than or equal to the inter-frame period) Dma transfer is restarted (the buffer must be
// copied by application code before the next transfer request from the timer (next pulse))
int main() {
  configureClocks(1000);
  initUsart1();
  print_clocks();

#if defined(STM32F1)
  gpioLed.init(GPIO_Mode_AF_PP);
  gpioCapture.init(GPIO_Mode_AF_OD);
  gpioLed.configRemap(GPIO_LED1_REMAP, ENABLE);
  gpioLed.configRemap(GPIO_CAPTURE_REMAP, ENABLE);
#elif defined(STM32F0) || defined(STM32F3)
  gpioLed.init(GPIO_Mode_OUT);
  gpioCapture.init(GPIO_Mode_AF);
  gpioCapture.configAF(GPIO_CAPTURE_AF);
#else
#error
#endif

  TIM_SelectInputTrigger(timerCapture.peripheral(), TIM_TS_TI1FP1);
  TIM_SelectSlaveMode(timerCapture.peripheral(), TIM_SlaveMode_Reset);

  dmaChannel.init((uint32_t) & (timerCapture.peripheral()->CCR2), (uint32_t)&fallCaptures[0], maxFrameLength, DMA_DIR_PeripheralSRC,
              DMA_PeripheralDataSize_HalfWord, DMA_MemoryDataSize_HalfWord, DMA_Mode_Normal, DMA_Priority_Medium,
              DMA_MemoryInc_Enable, DMA_PeripheralInc_Disable, DMA_M2M_Disable);

  TIM_DMACmd(timerCapture.peripheral(), TIM_DMA_CC2, ENABLE);

  timerCapture.setupCc2Callback(&cc2Callback);
  timerCapture.setupCc3Callback(&cc3Callback);
  timerCapture.init();

  timerCapture.interruptConfig(TIM_IT_CC2, ENABLE);

  // Note CCxS bits only writable when CCxE is 0 (channel is disabled)
  tcoFraming.init(TIM_OCMode_PWM1, DSHOT_TIMEOUT_COUNTS, TIM_OutputState_Enable);
  tciRising.init(TIM_ICPolarity_Rising, 0x0);
  tciFalling.init(TIM_ICPolarity_Falling, 0x0, TIM_ICPSC_DIV1, TIM_ICSelection_IndirectTI);

  dmaChannel.setEnabled(ENABLE);
  timerCapture.setEnabled(ENABLE);

#if defined(STM32F0)
  nvic_config(TIM2_BRK_UP_TRG_COM_IRQn, 0, ENABLE);
#elif defined(STM32F1)
  nvic_config(TIM2_UP_IRQn, 0, 0, ENABLE);
#elif defined(STM32F3)
  nvic_config(TIM2_IRQn, 0, 0, ENABLE);
#endif

  while (1) {
    gpioLed.toggle();
    mDelay(100);

    if (gotCapture) {
        gotCapture = false;
        uint16_t packet = 0;
        uint16_t throttle = 0;
        uint8_t telemRequest = 0;
        uint8_t csum = 0;
        for (uint8_t i = 0; i < frameLength; i++) {
            bool bit = fallCaptures[i] > dshotThreshold;
            packet = packet << 1;
            packet = packet | bit;
        }

        csum = packet & 0xf;
        packet = packet >> 4;
        telemRequest = packet & 0x1;
        throttle = packet >> 1;

        uint16_t csumCheck = 0;
        for (uint8_t i = 0; i < 3; i++) {
            csumCheck ^= packet;
            packet >>= 4;
        }

        csumCheck &= 0xf;

        printf("%d %d %d %d\r\n", throttle, telemRequest, csum, csumCheck);
                
        // get statistics on packets/second
        //printf("  %d  %d  %d\r\n", packet, decodedPackets, microseconds);
    }
  }

  return 0;
}

// Triggered on start of frame
void cc2Callback(void) {
  // enable framing interrupt to indicate end of frame on CC3 match
  timerCapture.interruptConfig(TIM_IT_CC3, ENABLE);
  // disable this interrupt until we are done with this frame
  timerCapture.interruptConfig(TIM_IT_CC2, DISABLE);
}

// triggered on timeout waiting for next bit in frame (end of frame)
void cc3Callback(void) {
  // On STM32F3, you don't have to use CC2 interrupt at the beginning of each frame
  // (acheiving one interrupt/frame instead of two interrupts/frame)
  // This can be accomplished by:
  // 1. leaving CC3 interrupt enabled (always)
  // 2. disabling the CC2 interrupt (always)
  // 3. using the TIM_SlaveMode_Combined_ResetTrigger instead of TIM_SlaveMode_Reset
  // 4. disabling the timer counter on each cc3 interrupt

  // setup interupt to re-enable framing interupt on the next rising edge (start of next frame)
  timerCapture.interruptConfig(TIM_IT_CC2, ENABLE);
  // Disable framing interrupt until we begin a new frame,
  // so that we don't keep firing while waiting for the next frame
  timerCapture.interruptConfig(TIM_IT_CC3, DISABLE);

  frameLength = maxFrameLength - DMA1_Channel3->CNDTR;
   // choose pulse-width of half the period for threshold of 0/1 decoding
  dshotThreshold = timerCapture.peripheral()->CCR1 / 2;

  dmaChannel.setEnabled(DISABLE);
  dmaChannel.peripheral()->CNDTR = maxFrameLength;
  dmaChannel.setEnabled(ENABLE);

  decodedPackets++;
  gotCapture = true;
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
  gpioUsart1Rx.init(GPIO_Mode_AF);
  gpioUsart1Tx.init(GPIO_Mode_AF);
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
