#include <io.h>

#include <ping-parser.h>
#include <ping-message.h>
#include <ping-message-common.h>
#include <ping-message-openesc.h>

extern "C" {
#include <adc.h>
#include <pwm-input.h>
#include <usart.h>
#include <throttle.h>
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
void io_initialize()
{
  ack.updateChecksum();
  nack.updateChecksum();
  registers[0] = 1;
  registers[1] = 2;
  registers[2] = 3;
}

void io_write_register(uint16_t address) {
  openesc_register m;
  m.set_address(address);
  m.set_value(((uint8_t*)&g)[address]);
  m.updateChecksum();
  // TODO this is not safe (non-copy)
  io_write_message(&m);
}

void io_write_register_multi(uint16_t address, uint16_t count) {
  openesc_register_multi m(count);
  m.set_address(address);
  m.set_data_length(count);
  for (uint16_t i = 0; i < m.data_length(); i++) {
    m.set_data_at(i, ((uint8_t*)&g)[address + i]);
  }
  m.updateChecksum();
  io_write_message(&m);
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
    case (OpenescId::SET_REGISTER):
    {
      openesc_set_register* m = (openesc_set_register*)message;
      if (m->address() > sizeof(g)) {
        io_write_message(&nack);
      } else {
        ((uint8_t*)&g)[m->address()] = m->value();
        io_write_register(m->address());
      }
    }
    break;
    case (OpenescId::READ_REGISTER):
    {
      openesc_read_register* m = (openesc_read_register*)message;
      if (m->address() > NUM_REGS) {
        io_write_message(&nack);
      } else {
        io_write_register(m->address());
      }
    }
    break;
    case (OpenescId::READ_REGISTER_MULTI):
    {
      openesc_read_register_multi* m = (openesc_read_register_multi*)message;
      if (m->address() + m->count() > sizeof(g)) {
        io_write_message(&nack);
      } else {
        io_write_register_multi(m->address(), m->count());
      }
    }
    break;
    case (OpenescId::SET_REGISTER_MULTI):
    {
      openesc_set_register_multi* m = (openesc_set_register_multi*)message;
      if (m->address() + m->data_length() > sizeof(g)) {
        io_write_message(&nack);
      } else {
        for (uint16_t i = 0; i < m->data_length(); i++) {
          ((uint8_t*)&g)[m->address() + i] = m->data()[i];
        }
        io_write_register_multi(m->address(), m->data_length());
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
        throttle_timeout_reset();
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
  message.set_phaseA(g.adc_buffer[ADC_IDX_PHASEA_VOLTAGE]);
  message.set_phaseB(g.adc_buffer[ADC_IDX_PHASEB_VOLTAGE]);
  message.set_phaseC(g.adc_buffer[ADC_IDX_PHASEC_VOLTAGE]);
  message.set_neutral(g.adc_buffer[ADC_IDX_NEUTRAL_VOLTAGE]);
  message.set_current(g.adc_buffer[ADC_IDX_BUS_CURRENT]);
  message.set_voltage(g.adc_buffer[ADC_IDX_BUS_VOLTAGE]);
  message.set_throttle(g.throttle);
  message.set_commutation_period(TIM_ARR(COMMUTATION_TIMER));
  message.updateChecksum();
  io_write_message(&message);
}
