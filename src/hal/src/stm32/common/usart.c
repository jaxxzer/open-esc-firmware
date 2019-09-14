#include <usart.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include <target.h>

#include <string.h>

// circular buffer
// TODO per instance buffers
// do not change, implementation takes advantage of integer overflow
const uint16_t _tx_buffer_size = 256;
static uint8_t _tx_buffer[256];
static uint8_t _tx_head;
static uint8_t _tx_tail;
static uint8_t _dma_transfer_count;

void usart_setup_dma_tx(uint32_t usart)
{
    rcc_periph_clock_enable(CONSOLE_TX_DMA_RCC);                                            // enable dma clock
    DMA_CPAR(CONSOLE_TX_DMA, CONSOLE_TX_DMA_CHANNEL) = (uint32_t)&USART_TDR(usart); // set CPAR
    dma_set_read_from_memory(CONSOLE_TX_DMA, CONSOLE_TX_DMA_CHANNEL);                              // set DIR
    dma_set_memory_size(CONSOLE_TX_DMA, CONSOLE_TX_DMA_CHANNEL, DMA_CCR_MSIZE_8BIT);              // set MSIZE
    dma_set_peripheral_size(CONSOLE_TX_DMA, CONSOLE_TX_DMA_CHANNEL, DMA_CCR_PSIZE_16BIT);          // set PSIZE
    dma_enable_memory_increment_mode(CONSOLE_TX_DMA, CONSOLE_TX_DMA_CHANNEL);                      // set MINC

    USART_CR3(usart) |= USART_CR3_DMAT;
    DMA_CCR(CONSOLE_TX_DMA, CONSOLE_TX_DMA_CHANNEL) |= DMA_CCR_TCIE;

    // lowest priority
    nvic_set_priority(CONSOLE_TX_DMA_IRQ, 0b11000000);
    nvic_enable_irq(CONSOLE_TX_DMA_IRQ);
}

void usart_initialize(uint32_t usart, uint32_t baudrate)
{
    // enable transmitter + receiver
    USART_CR1(usart) |= USART_CR1_TE | USART_CR1_RE;
    usart_set_baudrate(usart, baudrate);
    usart_enable(usart);

    usart_setup_dma_tx(usart);
}

// how many bytes are waiting to be shifted out
uint8_t usart_tx_waiting() {
    return _tx_head - _tx_tail;
}

// number of bytes waiting or number of bytes till the end of the buffer
// whichever is smallest
uint8_t usart_tx_dma_waiting() {
    if (_tx_head >= _tx_tail) {
        return _tx_head - _tx_tail;
    } else {
        return _tx_buffer_size - _tx_tail;
    }
}

// how many bytes available to write to buffer
uint8_t usart_tx_available() {
    return _tx_buffer_size - usart_tx_waiting() - 1; // must subtract one for tracking empty/patrial[255]/full states
}

void usart_start_tx_dma_transfer(uint32_t usart)
{
    if (DMA_CCR(CONSOLE_TX_DMA, CONSOLE_TX_DMA_CHANNEL) & DMA_CCR_EN) {
        // dma busy doing transfer
        return;
    }

    _dma_transfer_count = usart_tx_dma_waiting(usart);
    if (_dma_transfer_count) {
        // we must clear the flag first or we will hang sporadically
        DMA_IFCR(CONSOLE_TX_DMA) |= DMA_IFCR_CTCIF(CONSOLE_TX_DMA_CHANNEL);
        DMA_CNDTR(CONSOLE_TX_DMA, CONSOLE_TX_DMA_CHANNEL) = _dma_transfer_count;
        DMA_CMAR(CONSOLE_TX_DMA, CONSOLE_TX_DMA_CHANNEL) = (uint32_t)(_tx_buffer + _tx_tail);
        USART_ICR(CONSOLE_USART) |= USART_ICR_TCCF;
        DMA_CCR(CONSOLE_TX_DMA, CONSOLE_TX_DMA_CHANNEL) |= DMA_CCR_EN;
    }
}

void usart_dma_transfer_complete_isr(uint32_t usart)
{
    // disable dma transfer
    DMA_CCR(CONSOLE_TX_DMA, CONSOLE_TX_DMA_CHANNEL) &= ~DMA_CCR_EN;

    // free available space, move head forward
    _tx_tail += _dma_transfer_count;

    // start transferring fresh data
    usart_start_tx_dma_transfer(usart);
}

void usart_write(uint32_t usart, const char* data, uint16_t length)
{
    // no blocking
    if (usart_tx_available() < length) {
        return;
    }

    // copy data to tx buffer
    for (uint16_t i = 0; i < length; i++) {
        _tx_buffer[_tx_head++] = data[i];
    }

    // start transferring the data
    usart_start_tx_dma_transfer(usart);
}

void usart_write_string(uint32_t usart, const char* string)
{
    usart_write(usart, string, strlen(string));
}

void usart_write_int(uint32_t usart, uint32_t i)
{
  // output character buffer
  char c[10];
  // maximum character length for uint32_t
  uint8_t len = 10;

  // first character
  uint8_t p = i % 10;
  c[--len] = p + '0';

  while (i > 9) {
    i /= 10;
    p = i % 10;
    c[--len] = p + '0';
  }

  usart_write(usart, &c[len], 10 - len);
}
