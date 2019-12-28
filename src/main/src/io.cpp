#include <io.h>

#include <ping-parser.h>
#include <ping-message.h>
#include <ping-message-common.h>
#include <ping-message-openesc.h>

extern "C" {
#include <adc.h>
#include <pwm-input.h>
#include <usart.h>
}

#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>

#include <target.h>
#include <global.h>

PingParser parser;

common_ack ack;
common_nack nack(32);

#define NUM_REGS 32
uint32_t registers[NUM_REGS];

extern "C"
void io_control_timeout_isr()
{
  g.throttle_valid = false;
}

// this is used only when pwm signal is not present
// pwm input driver manages the timeout itself
void io_control_timeout_reset()
{
  TIM_CNT(PWM_INPUT_TIMER) = 1;
}

void io_initialize()
{
  ack.updateChecksum();
  nack.updateChecksum();
  registers[0] = 1;
  registers[1] = 2;
  registers[2] = 3;
}

void io_handle_message(ping_message* message)
{
  switch (message->message_id()) {
    case (CommonId::GENERAL_REQUEST):
    {
      common_general_request* m = (common_general_request*)message;
      switch(m->requested_id()) {
        case (CommonId::DEVICE_INFORMATION):
         {
          common_device_information response;
          response.set_device_type(3);
          response.set_device_revision(0);
          response.set_firmware_version_major(0);
          response.set_firmware_version_minor(0);
          response.set_firmware_version_patch(1);
          response.updateChecksum();
          io_write_message(&response);
        }
        break;
        case (CommonId::PROTOCOL_VERSION):
         {
          common_protocol_version response;
          response.set_version_major(1);
          response.set_version_minor(0);
          response.set_version_patch(0);
          response.updateChecksum();
          io_write_message(&response);
        }

        break;

        default:
        break;
      }
    }
    break;
    case (OpenescId::SET_THROTTLE):
    {
      openesc_set_throttle* m = (openesc_set_throttle*)message;
      if (m->throttle_signal() > 0xfff) {
        g.throttle_valid = false;
        io_write_message(&nack);
      } else {
        g.throttle_valid = true;
        g.throttle = m->throttle_signal();
        io_control_timeout_reset();
        io_write_state();
      }
    }
    break;
    default:
    break;
  }
}

void io_process_input()
{
  char b;
  uint8_t nbytes = usart_rx_waiting();
  for (uint8_t i = 0; i < nbytes; i++) {
    usart_read(CONSOLE_USART, &b, 1);
    io_parse_byte(b);
  }
}

void io_parse_byte(uint8_t b)
{
  if (parser.parseByte(b) == PingParser::NEW_MESSAGE) {
    io_handle_message(&parser.rxMessage);
  }
}

void io_parse_reset()
{
  parser.reset();
}

void io_write_message(ping_message* message)
{
  usart_write(CONSOLE_USART, (char*)message->msgData, message->msgDataLength());
}

void io_write_state()
{
  static openesc_state message;
#if defined(ADC_IDX_PHASEA_VOLTAGE)
  message.set_phaseA(g.adc_buffer[ADC_IDX_PHASEA_VOLTAGE]);
#else
  message.set_phaseA(0);
#endif
#if defined(ADC_IDX_PHASEB_VOLTAGE)
  message.set_phaseB(g.adc_buffer[ADC_IDX_PHASEB_VOLTAGE]);
#else
  message.set_phaseB(0);
#endif
#if defined(ADC_IDX_PHASEC_VOLTAGE)
  message.set_phaseC(g.adc_buffer[ADC_IDX_PHASEC_VOLTAGE]);
#else
  message.set_phaseC(0);
#endif
#if defined(ADC_IDX_NEUTRAL_VOLTAGE)
  message.set_neutral(g.adc_buffer[ADC_IDX_NEUTRAL_VOLTAGE]);
#else
  message.set_neutral(0);
#endif
#if defined(ADC_IDX_BUS_CURRENT)
  message.set_current(g.adc_buffer[ADC_IDX_BUS_CURRENT]);
#else
  message.set_current(0);
#endif
#if defined(ADC_IDX_BUS_VOLTAGE)
  message.set_voltage(g.adc_buffer[ADC_IDX_BUS_VOLTAGE]);
#else
  message.set_voltage(0);
#endif
  message.set_throttle(g.throttle);
  message.set_commutation_period(TIM_ARR(COMMUTATION_TIMER));
  message.updateChecksum();
  io_write_message(&message);
}
