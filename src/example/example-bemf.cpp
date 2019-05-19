// six-step comparator bemf detection example
// the comparator state is polled in this example, and at each zero-cross event,
// the input pin is reconfigured for the next undriven phase
// demonstrates comparator states during six step commutation
// spin a motor by hand and a gpio is pulsed at each zero cross
#include "stm32lib-conf.h"

Gpio gpioComparator0 { GPIO_PORT_BEMF, GPIO_PIN_BEMF_U };
Gpio gpioComparator1 { GPIO_PORT_BEMF, GPIO_PIN_BEMF_V };
Gpio gpioComparator2 { GPIO_PORT_BEMF, GPIO_PIN_BEMF_W };
Gpio gpioComparatorN { GPIO_PORT_BEMF, GPIO_PIN_BEMF_NEUTRAL };

// this pin is used for output, it is toggled on each comparator match (zero cross)
Gpio gpioTest1 { GPIO_USART1_TX_PORT, GPIO_USART1_TX_PIN };

// this array holds the desired values of the comparator register for each commutation step
// NOTE: the pin selection should cycle through each phase, as in (A-B-C-A-B-C)
// however, this does not work as expected on GD32, and the correct sequence appears to be instead (A-A-B-B-C-C)
// my best explanation moment for why this works is a silicon error. the issue appears to be only with the falling
// edge configurations.
static const uint32_t compRegs[6] = {
  0x00000041, // rising edge
  0x00000861, // invert comparator output (falling edge)
  0x00000061, // rising edge
  0x00000851, // invert comparator output (falling edge)
  0x00000051, // rising edge
  0x00000841, // invert comparator output (falling edge)
};

// This is what the correct comparator configuration order should be according to the register documentation
// static const uint32_t compRegs[6] = {
//   0x00000041, // rising edge
//   0x00000851, // invert comparator output (falling edge)
//   0x00000061, // rising edge
//   0x00000841, // invert comparator output (falling edge)
//   0x00000051, // rising edge
//   0x00000861, // invert comparator output (falling edge)
// };

// the current commutation step
static uint8_t comStep = 0;

// get comparator output
bool comparatorGetOuptut(uint32_t baseAddress);

// advance to the next commutation step configuration
void nextCompare();

int main() {
  configureClocks(1000);

  gpioComparator0.init(GPIO_Mode_AN);
  gpioComparator1.init(GPIO_Mode_AN);
  gpioComparator2.init(GPIO_Mode_AN);
  gpioComparatorN.init(GPIO_Mode_AN);

  gpioTest1.init(GPIO_Mode_OUT);

  // Initialize the comparator
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  *(volatile uint32_t *) COMP_BASE = compRegs[comStep];

  while (1) {
    // wait for zero cross
    while (comparatorGetOuptut(COMP_BASE));

    // set up to catch the next zero cross in sequence
    // configure comparator for the next step
    // the proper direction of rotation for the rotor is hard coded here
    // turn the rotor both ways and note the difference in the output
    // replace this line with comStep-- to reverse the operation
    comStep++;
    comStep = comStep % 6;
    *(volatile uint32_t *) COMP_BASE = compRegs[comStep];

    // toggle gpio, hook up a logic analyzer to see
    gpioTest1.toggle();
  }

  return 0;
}

#define COMP_MASK_OUTPUT (1 << 14)
bool comparatorGetOuptut(uint32_t baseAddress) {
  return *(volatile uint32_t *) baseAddress & COMP_MASK_OUTPUT;
}
