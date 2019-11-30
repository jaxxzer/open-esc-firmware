#include <audio.h>

extern "C"
{
#include <bridge.h>
#include <watchdog.h>
}

#include <io.h>

void audio_play_note_blocking(uint16_t frequency, uint8_t duty, uint32_t cycles)
{
  bridge_enable();
  bridge_set_state(BRIDGE_STATE_AUDIO);
  bridge_set_audio_duty(duty);
  bridge_set_audio_frequency(frequency);
  for (uint32_t i = 0; i < cycles; i++) { watchdog_reset(); io_process_input(); }
  bridge_disable();
}
