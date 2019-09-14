#pragma once

#include <inttypes.h>

void commutation_isr();
void comparator_unblank_isr();
void comparator_zc_isr();
void comparator_zc_timeout_isr();
void comparator_blank_complete_isr();
void usart_dma_transfer_complete_isr(uint32_t usart);
