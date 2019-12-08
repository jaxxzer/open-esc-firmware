extern "C" {
#include <console.h>
#include <global.h>
#include <system.h>
}

#include <target.h>

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

int main(void) {
  system_initialize();
  console_initialize();
  while (1) {
    console_write("hello world\r\n");
  }
}
