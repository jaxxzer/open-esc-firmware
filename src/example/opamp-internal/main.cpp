extern "C" {
#include <adc.h>
#include <console.h>
#include <global.h>
#include <system.h>


}

#include <target.h>

#if defined(OPAMP_INTERNAL)
#include <libopencm3/stm32/g4/opamp.h>
#endif
#include <libopencm3/stm32/rcc.h>

// declared/documented in global.h
// todo remove these irrelevant definitions
// build requires them for now
const uint16_t slow_run_zc_confirmations_required = 14;
uint16_t run_zc_confirmations_required = slow_run_zc_confirmations_required;
volatile bool starting;
const uint16_t startup_commutation_period_ticks = 12000;
const uint16_t startup_zc_confirmations_required = 15;
volatile uint32_t zc_counter;
uint32_t zc_confirmations_required = startup_zc_confirmations_required;

// opamp connections
// opamp1
// -PA3 (VINM0)
// +PA1 (VINP0)
// out ADC1_IN3, ADC1_IN13* (*OPAINTOEN must be set)
int main(void) {
  system_initialize();
  console_initialize();
  adc_initialize();
  adc_start();

#if defined(OPAMP_INTERNAL)
  rcc_periph_clock_enable(RCC_SYSCFG);
  
  // select pga mode on inverting input
  OPAMP_CSR(OPAMP1) |= OPAMP_CSR_VM_SEL_PGA;
  // non inverting gain = 2 with VINM0 pin for input bias
  OPAMP1_CSR |= (0b10000 << OPAMP_CSR_PGA_GAIN_SHIFT);
  // connect opamp vp to VINP0 signal
  OPAMP1_CSR |= (0b00 << OPAMP_CSR_VP_SEL_SHIFT);
  // connect the opamp internally
  OPAMP1_CSR |= OPAMP_CSR_OPAINTOEN;
  // enable high speed mode
  OPAMP1_CSR |= OPAMP_CSR_OPAHSM;
  // enable the opamp
  OPAMP1_CSR |= OPAMP_CSR_OPAEN;


  while (1) {
    console_write("opamp adc value is: ");
    console_write_int(g.adc_buffer[ADC_IDX_PHASEA_VOLTAGE]);
    console_write("\r\n");
  }
#else
  while (1) {
    console_write("internal opamp not supported\r\n");
  }
#endif
}
