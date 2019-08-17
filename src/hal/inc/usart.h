#pragma once

#include <inttypes.h>

void usart_initialize(uint32_t usart, uint32_t baudrate);
void usart_write(uint32_t usart, const char* string);
void usart_write_data(uint32_t usart, const char* data, uint16_t length);
void usart_write_int(uint32_t usart, uint32_t i);
