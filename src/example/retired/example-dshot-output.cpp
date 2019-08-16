// This example transmits a dshot symbol using dma
// The timer period is set to the duration of the dshot frame
//
// The bit pulse widths are written to an array, then dma transfers
// each bit to the compare register sequentially on each timer update event
//
// The array must be null-terminated in order to set 0 duty on the signal
// line at the end of a frame

#include "stm32lib-conf.h"

#define THROTTLE_BITS 11
#define FRAME_BITS 16
#define DSHOT_TIMER timer2
#define DSHOT_TIMER_CHANNEL TIM_Channel_1
#define DSHOT_DMA_CHANNEL DMA1_Channel2
#define DSHOT_GPIO_PORT GPIOA
#define DSHOT_GPIO_PIN 15
#define DSHOT_GPIO_AF 2

Timer &dshotTimer = DSHOT_TIMER ;
TimerChannelOutput tco { &dshotTimer, DSHOT_TIMER_CHANNEL };
Dma dshotDma { DSHOT_DMA_CHANNEL };
Gpio dshotGpio { DSHOT_GPIO_PORT, DSHOT_GPIO_PIN };

// pulse widths, in timer counts
static const uint16_t dshot_0 = 20000;
static const uint16_t dshot_1 = 2 * dshot_0;
static const uint16_t dshot_period = 3 * dshot_0; // symbol duration

// arbitrary data (for now)
uint16_t pulses[FRAME_BITS];

void updatePulses();
uint16_t prepareDshotPacket(uint16_t throttle);

int main() {
  configureClocks(1000);

  dshotGpio.init(GPIO_Mode_AF);
  dshotGpio.configAF(DSHOT_GPIO_AF);

  dshotTimer.init(2, dshot_period);
  tco.init(TIM_OCMode_PWM1, 0, TIM_OutputState_Enable);
  tco.preloadConfig(ENABLE);

  dshotDma.init((uint32_t) & (dshotTimer.peripheral()->CCR1), (uint32_t)&pulses[0], FRAME_BITS, DMA_DIR_PeripheralDST,
              DMA_PeripheralDataSize_Word, DMA_MemoryDataSize_HalfWord, DMA_Mode_Normal, DMA_Priority_Medium,
              DMA_MemoryInc_Enable, DMA_PeripheralInc_Disable, DMA_M2M_Disable);

  dshotDma.setEnabled(ENABLE);


  TIM_DMACmd(dshotTimer.peripheral(), TIM_DMA_Update, ENABLE);

  dshotTimer.setEnabled(ENABLE);
  dshotTimer.setMOE(ENABLE);

  // try it out if this example does not work as you expect
  // print_clocks();

  while (1) {
    mDelay(100);
    updatePulses();

    // reload dma number of data to transfer
    // it will go out on next timer update
    dshotDma.setEnabled(DISABLE);
    DMA1_Channel2->CNDTR = sizeof(pulses) / 2;
    dshotDma.setEnabled(ENABLE);
  }

  return 0;
}

void updatePulses()
{
  static uint8_t throttle;
  static uint8_t bitPosition;
  throttle++;
  throttle %= 1 << THROTTLE_BITS;

  uint16_t packet = prepareDshotPacket(throttle);
  for (bitPosition = 0; bitPosition < THROTTLE_BITS; bitPosition++)
  {
    if (packet & (1 << bitPosition)) {
      pulses[bitPosition] = dshot_1;
    } else {
      pulses[bitPosition] = dshot_0;
    }
  }

}

// checksum calculation
// thanks a lot, BETAFLIGHT
uint16_t prepareDshotPacket(uint16_t throttle)
{
  static const bool telemetry = false;
  uint16_t packet = (throttle << 1) | telemetry;

  // compute checksum
  int csum = 0;
  int csum_data = packet;
  for (int i = 0; i < 3; i++) {
      csum ^=  csum_data;   // xor data by nibbles
      csum_data >>= 4;
  }
  csum &= 0xf;
  // append checksum
  packet = (packet << 4) | csum;

  return packet;
}
