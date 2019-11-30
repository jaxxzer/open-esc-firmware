#pragma once

#include <inttypes.h>

void usart_initialize(uint32_t usart, uint32_t baudrate);
uint16_t usart_read(uint32_t usart, char* byte, uint16_t length);
uint16_t usart_rx_waiting();
void usart_write(uint32_t usart, const char* data, uint16_t length);
void usart_write_string(uint32_t usart, const char* string);
void usart_write_int(uint32_t usart, uint32_t i);
