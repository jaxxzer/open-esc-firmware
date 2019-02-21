// Audio example
// If this program is difficult to understand, start with the beep example
// 
// This program plays an 8 bit 8khz pcm wav audio file
// The file is not parsed, the header data is played like the rest of the file (so brief, it's not noticable)
//
// The program is configured to produce this result on the motor phase timer:
// 
//                |------------------------------------------------------------------|
// timer overflow |        |        |        |        |        |        |        |   |
// ccr1 output    |‾‾‾‾‾|___________|‾‾‾|_____________|‾‾‾‾‾|___________|‾‾|_________|
// ccr2 output    |________|‾|_______________|‾|_______________|‾|_______________|‾|_|
// ccr3 output    |__________________________________________________________________|
//
// Here, the audio pcm data is loaded into CCR1. CCR2 is loaded with a constant (small)
// value. The phases are activated alternately in order to make the rotor move back and
// forth. The amplitude of the vibration is dynamically adjusted by changing the CCR1 value
// The timer overflow frequency must be more than 4x the maximum frequency in the audio
// to prevent ailiasing. (Because the pwm is alternated between phases, the effective
// pwm frequency is half the timer overflow frequency.)

#include "stm32lib-conf.h"

// pick any header from the wav directory
#include "wav/blue.h"

Timer &timerMotor = PHASE_OUTPUT_TIMER;
Gpio gpio_m1{ GPIO_PORT_HIGH_U, GPIO_PIN_HIGH_U };
Gpio gpio_m1n{ GPIO_PORT_LOW_U, GPIO_PIN_LOW_U };
Gpio gpio_m2{ GPIO_PORT_HIGH_V, GPIO_PIN_HIGH_V };
Gpio gpio_m2n{ GPIO_PORT_LOW_V, GPIO_PIN_LOW_V };
Gpio gpio_m3{ GPIO_PORT_HIGH_W, GPIO_PIN_HIGH_W };
Gpio gpio_m3n{ GPIO_PORT_LOW_W, GPIO_PIN_LOW_W };

TimerChannelOutput tco1{&timerMotor, TIM_Channel_1};
TimerChannelOutput tco2{&timerMotor, TIM_Channel_2};
TimerChannelOutput tco3{&timerMotor, TIM_Channel_3};

// This timer updates the amplitude at 8kHz
Timer &timerAudio = timer15;

// Buffer to hold CCR1, CCR2 values in two states
// index 1 is updated with sequential audio samples at 8kHz
uint16_t pulses[] = { 0, 0, 200, 0 };

int main() {
  configureClocks(1000);

  // Configure gpio
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

  // Configure dma channel to transfer pulse lengths from `pulses` array to timer CCR1 and CCR2
  // This is done via the 'dma burst' feature of the timer (DMAR)
  Dma dmaChannel = Dma(DMA1_Channel4);
  dmaChannel.init((uint32_t) & (timerMotor.peripheral()->DMAR), (uint32_t)&pulses[0], 4, DMA_DIR_PeripheralDST,
              DMA_PeripheralDataSize_HalfWord, DMA_MemoryDataSize_HalfWord, DMA_Mode_Circular, DMA_Priority_Medium,
              DMA_MemoryInc_Enable, DMA_PeripheralInc_Disable, DMA_M2M_Disable);
  dmaChannel.setEnabled(ENABLE);

  // phase output timer configuration
  // PSC = 0, ARR = 1000, pwm frequency is 72kHz, allowing max audio frequency of 18kHz
  // reduce ARR (minimum 255) to increase duty to the motor during audio playback
  // this will increase volume, but also heating!
  timerMotor.init(0, 1000);
  timerMotor.setDTG(0x18); // ~333ns deadtime insertion (this also sets OSSR)

  // the dma request may be an unused CC (set CCR to 0 for equivalent to update request) or Update (pick what's available)
  TIM_DMACmd(timerMotor.peripheral(), TIM_DMA_CC4, ENABLE);
  TIM_DMAConfig(timerMotor.peripheral(), TIM_DMABase_CCR1, TIM_DMABurstLength_2Transfers);

  tco1.init(TIM_OCMode_PWM1, 0, TIM_OutputState_Enable, TIM_OutputNState_Enable);
  tco2.init(TIM_OCMode_PWM1, 0, TIM_OutputState_Enable, TIM_OutputNState_Enable);
  tco3.init(TIM_OCMode_PWM1, 0, TIM_OutputState_Enable, TIM_OutputNState_Enable);

  timerMotor.setMOE(ENABLE);
  timerMotor.setEnabled(ENABLE);

  // This dma channel moves samples from the audio buffer to the 'pulses' array used by the other dma channel
  // The samples are then moved into the timer CCR1 register by the other dma channel
  Dma dmaChannelAudio = Dma(DMA1_Channel5);
  dmaChannelAudio.init((uint32_t) & pulses[1], (uint32_t)&wav[0], sizeof(wav), DMA_DIR_PeripheralDST,
              DMA_PeripheralDataSize_HalfWord, DMA_MemoryDataSize_Byte, DMA_Mode_Normal, DMA_Priority_Medium,
              DMA_MemoryInc_Enable, DMA_PeripheralInc_Disable, DMA_M2M_Disable);
  dmaChannelAudio.setEnabled(ENABLE);

  // this timer triggers dmaChannelAudio to transfer samples from the audio buffer at 8kHz
  timerAudio.initFreq(8000);

  // make dma request on update event
  TIM_DMACmd(timerAudio.peripheral(), TIM_DMA_Update, ENABLE);

  // begin transferring audio samples
  timerAudio.setEnabled(ENABLE);

  // wait for audio to complete
  while (!DMA_GetFlagStatus(DMA1_FLAG_TC5));

  timerAudio.setEnabled(DISABLE);

  // freewheel
  tco1.setEnabledN(DISABLE);
  tco2.setEnabledN(DISABLE);
  tco3.setEnabledN(DISABLE);
  timerMotor.setEnabled(DISABLE);

  while(1);
  return 0;
}
