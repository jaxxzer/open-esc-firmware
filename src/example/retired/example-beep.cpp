// Beep example
// Use a dma channel to update timer CCR1 and CCR2 on update events
// Pulse them alternately to make the rotor waggle back and forth
// OC preload enabled!
// Timer makes dma request on each overflow and dma updates CCR1
// and CCR2 (preloaded, so the new values are used on the subsequent overflow)

#include "stm32lib-conf.h"

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

// If dma transfer length (CNDTR) is 4, then alternate CCR1 and CCR2 values (400, 0)
// If dma transfer length is 8 or 12, then a consistent pulse length is not used (500, 100 also)
// This is to modulate some small noise in the waveform for pleasant effect
uint16_t pulses[] = { 0, 400, 400, 0, 0, 500, 500, 0, 0, 100, 100, 0 };
uint16_t notes[] = { 9000, 10000, 6000, 6500, 8000 };

int main() {
  configureClocks(1000);

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

  // Configure dma channel to transfer pulse lengths from `pulses` array to timer CCR1 and CCR2
  // This is done via the 'dma burst' feature of the timer (DMAR)
  Dma dmaChannel = Dma(DMA1_Channel4);

  // move values from pulses array to timer DMAR register
  dmaChannel.init((uint32_t) & (timer.peripheral()->DMAR), (uint32_t)&pulses[0], 12, DMA_DIR_PeripheralDST,
              DMA_PeripheralDataSize_HalfWord, DMA_MemoryDataSize_HalfWord, DMA_Mode_Circular, DMA_Priority_Medium,
              DMA_MemoryInc_Enable, DMA_PeripheralInc_Disable, DMA_M2M_Disable);

  dmaChannel.setEnabled(ENABLE);

  // Timer configuration
  timer.initFreq(notes[0]);
  timer.setDTG(0x18); // ~333ns deadtime insertion

  // the dma request may be an unused CC (set CCR to 0 for equivalent to update request)
  // or Update (pick what's available)
  TIM_DMACmd(timer.peripheral(), TIM_DMA_CC4, ENABLE);

  // timer generates two DMA requests, CCR1 and CCR2 are updated
  TIM_DMAConfig(timer.peripheral(), TIM_DMABase_CCR1, TIM_DMABurstLength_2Transfers);
  
  // enable CCR preload
  TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
  TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);

  tco1.init(TIM_OCMode_PWM1, 0, TIM_OutputState_Enable, TIM_OutputNState_Enable);
  tco2.init(TIM_OCMode_PWM1, 0, TIM_OutputState_Enable, TIM_OutputNState_Enable);
  tco3.init(TIM_OCMode_PWM1, 0, TIM_OutputState_Enable, TIM_OutputNState_Enable);

  timer.setEnabled(ENABLE);
  timer.setMOE(ENABLE);

  // play beep sequence
  for (uint8_t n = 0; n < 5; n++) {
    timer.setFrequency(notes[n]);
    mDelay(75);
  }

  // freewheeling
  tco1.setEnabledN(DISABLE);
  tco2.setEnabledN(DISABLE);
  tco3.setEnabledN(DISABLE);
  timer.setEnabled(DISABLE);

  while(1);

  return 0;
}
