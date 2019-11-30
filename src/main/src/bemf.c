#include <bemf.h>

#include <global.h>

#include <target.h>

// stub
void bemf_initialize()
{
}

// stub
uint16_t bemf_get_phase_voltage(bemf_phase_e phase)
{
// todo move compile switch to cmake build system
#if defined(FEEDBACK_BEMF)
    switch (phase) {
        case (BEMF_PHASE_A):
            return g.adc_buffer[ADC_CHANNEL_PHASEA_VOLTAGE];
        case (BEMF_PHASE_B):
            return g.adc_buffer[ADC_CHANNEL_PHASEB_VOLTAGE];
        case (BEMF_PHASE_C):
            return g.adc_buffer[ADC_CHANNEL_PHASEC_VOLTAGE];
        case (BEMF_PHASE_N):
            return g.adc_buffer[ADC_CHANNEL_NEUTRAL_VOLTAGE];
    }
#endif
}
