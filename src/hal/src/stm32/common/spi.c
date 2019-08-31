#include <spi.h>

#include <libopencm3/stm32/spi.h>

void spi_initialize(uint32_spi)
{
    spi_init_master(spi,
                    SPI_CR1_BAUDRATE_FPCLK_DIV_8,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1,
                    SPI_CR1_MSBFIRST);
                    
}
