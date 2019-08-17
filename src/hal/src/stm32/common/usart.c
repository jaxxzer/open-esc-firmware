#include <usart.h>

#include <libopencm3/stm32/usart.h>
#include <target.h>

void usart_initialize(uint32_t usart, uint32_t baudrate)
{
    // enable transmitter + receiver
    USART_CR1(usart) |= USART_CR1_TE | USART_CR1_RE;
    usart_set_baudrate(usart, baudrate);
    usart_enable(usart);
}

void usart_write(uint32_t usart, const char* string)
{
    int i = 0;
    uint8_t c = 0;
    do {
        c = string[i++];
        usart_send_blocking(usart, c);
    } while(c != 0);
}

void usart_write_data(uint32_t usart, const char* data, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++) {
        usart_send_blocking(usart, data[i]);
    }
}

void usart_write_int(uint32_t usart, uint32_t i)
{
  char c[10];
  uint8_t len = 10;

  uint8_t p = i % 10;
  c[--len] = p + '0';
  i -= p; // not neccessary? taken care of with integer division

  while (i) {
    i /= 10;
    p = i % 10;
    c[--len] = p + '0';
    i -= p; // not neccessary? taken care of with integer division
  }
  usart_write_data(usart, &c[len], 10 - len);
}
