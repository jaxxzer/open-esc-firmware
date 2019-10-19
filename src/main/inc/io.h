#pragma once

#include <inttypes.h>
#include <ping-message.h>

void io_initialize();
void io_handle_message(ping_message* message);
void io_parse_byte(uint8_t b);
void io_parse_reset();
void io_process_input();
void io_write_message(ping_message* message);
void io_write_state();
