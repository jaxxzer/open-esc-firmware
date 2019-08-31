#pragma once


// spi is implemented with circular tx and rx buffers
void spi_initialize(uint32_t spi);
void spi_set_clock_speed(uint32_t spi);

void spi_set_rx_buffer_size(uint32_t spi, uint16_t size);
void spi_set_tx_buffer_size(uint32_t spi, uint16_t size);
void spi_transfer(uint32_t spi, const uint8_t* byte);

uint16_t spi_write_data(uint32_t spi, const char* data, uint16_t length);
uint16_t spi_read_data(uint32_t spi, char* data, uint16_t length);
