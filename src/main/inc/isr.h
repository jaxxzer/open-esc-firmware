#pragma once

#include <inttypes.h>

void io_control_timeout_isr();
void commutation_isr();
void comparator_unblank_isr();
void comparator_zc_isr();
void comparator_zc_timeout_isr();
void comparator_blank_complete_isr();
void usart_dma_transfer_complete_isr(uint32_t usart);
void overcurrent_watchdog_isr();
