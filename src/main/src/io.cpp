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

PingParser parser;

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
  message.set_phaseA(adc_buffer[ADC_IDX_PHASEA_VOLTAGE]);
  message.set_phaseB(adc_buffer[ADC_IDX_PHASEB_VOLTAGE]);
  message.set_phaseC(adc_buffer[ADC_IDX_PHASEC_VOLTAGE]);
  message.set_neutral(adc_buffer[ADC_IDX_NEUTRAL_VOLTAGE]);
  message.set_current(adc_buffer[ADC_IDX_BUS_CURRENT]);
  message.set_voltage(adc_buffer[ADC_IDX_BUS_VOLTAGE]);
  message.set_throttle(pwm_input_get_throttle());
  message.set_commutation_frequency(TIM_ARR(COMMUTATION_TIMER));
  message.updateChecksum();
  io_write_message(&message);
}
