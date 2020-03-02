extern "C" {
#include <adc.h>
#include <bridge.h>
#include <global.h>
#include <overcurrent-watchdog.h>
#include <system.h>
#include <watchdog.h>
}

// hpp
#include <audio.h>

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

  // safety stuff
  adc_initialize();
  overcurrent_watchdog_initialize();
  adc_start();
  watchdog_start(10); // 10ms watchdog timeout

  // enable bridge
  bridge_initialize();
  bridge_set_run_duty(100);

  for (uint8_t i = 0; i < 6; i++)
  {
    bridge_commutate();
    watchdog_reset();
  }

  while (1)
  {
    watchdog_reset();
  }
}
