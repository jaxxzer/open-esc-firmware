#include <comparator.h>
#include <debug-pins.h>
#include <global.h>
#include <target.h>

#include <libopencm3/stm32/timer.h>

// these comparator isrs are defined by the application

// zc timer isr
void comparator_zc_timeout_isr() {
  // timeout waiting for zero-cross, go to open loop
  // TODO should load commutation timer default values
  starting = true;
  debug_pins_toggle2();
  debug_pins_toggle2();
}

void comparator_zc_isr()
{
  while (zc_counter--) {
    if (!comparator_get_output()) {
      zc_counter = zc_confirmations_required;
      comparator_blank(20000); // give main loop a gasp of air
      return;
    }
    debug_pins_toggle0();
  }

  volatile uint16_t cnt = TIM_CNT(ZC_TIMER);
  // TODO does zero cause update event?
  TIM_CNT(ZC_TIMER) = 0;


  if (cnt < 300) {
    starting = true;
  }
  if (cnt < TIM_ARR(COMMUTATION_TIMER)/2) {
    if (starting) {
      zc_confirmations_required = startup_zc_confirmations_required;

      zc_counter = zc_confirmations_required;
      TIM_ARR(COMMUTATION_TIMER) = startup_commutation_period_ticks;
      debug_pins_toggle1();
      debug_pins_toggle1();
    }
  } else if (cnt > TIM_ARR(COMMUTATION_TIMER) + TIM_ARR(COMMUTATION_TIMER)/2 + TIM_ARR(COMMUTATION_TIMER)/4) {
    // we missed a cross
    // do nothing
  } else {
    if (starting) {
      starting = false;
      debug_pins_toggle1();
    } else {
      if (cnt < 6000) {
        if (run_zc_confirmations_required > 6) {
          run_zc_confirmations_required--;
        }
      } else if (cnt < 8000) {
        if (run_zc_confirmations_required > 8) {
          run_zc_confirmations_required--;
        }
      } else if (cnt < 10000) {
        if (run_zc_confirmations_required > 10) {
          run_zc_confirmations_required--;
        }
      } else {
        if (run_zc_confirmations_required < slow_run_zc_confirmations_required) {
          run_zc_confirmations_required++;
        }
      }
      zc_confirmations_required = run_zc_confirmations_required;
      TIM_ARR(COMMUTATION_TIMER) = TIM_ARR(COMMUTATION_TIMER)/2 + TIM_ARR(COMMUTATION_TIMER)/4 + cnt/4;
      // TIM_ARR(COMMUTATION_TIMER) = TIM_ARR(COMMUTATION_TIMER)/2 + cnt/2;
      // schedule commutation (+ timing advance)
      TIM_CNT(COMMUTATION_TIMER) = TIM_ARR(COMMUTATION_TIMER)/2;
      debug_pins_toggle1();
      debug_pins_toggle1();
      debug_pins_toggle1();
    }
  }

  // blanking period
  TIM_CCR1(COMMUTATION_TIMER) = TIM_ARR(COMMUTATION_TIMER)/8;

  comparator_zc_isr_disable();
  TIM_SR(COMMUTATION_TIMER) = 0;
}
