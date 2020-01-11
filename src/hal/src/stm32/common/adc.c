#include <adc.h>

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#if defined(STM32G0) || defined(STM32G4)
#include <libopencm3/stm32/g0/dmamux.h>
#endif

#include <target.h>
#include <global.h>

void adc_initialize_dmamux(uint32_t dmamux, uint32_t dma_channel, uint8_t reqid)
{
#if defined(STM32G0) || defined(STM32G4)
    DMAMUX_CxCR(dmamux, dma_channel) = reqid;
#endif
}

void adc_initialize_dma(uint32_t rcc, uint32_t dma, uint8_t dma_channel, uint32_t buffer, uint16_t buffer_size)
{
    rcc_periph_clock_enable(rcc);

    dma_set_peripheral_address(dma, dma_channel, (uint32_t)&ADC_DR(ADC_PERIPH)); // set CPAR
    dma_set_memory_address(dma, dma_channel, buffer);             // set CMAR
    dma_set_read_from_peripheral(dma, dma_channel);                              // set DIR
    dma_set_memory_size(dma, dma_channel, DMA_CCR_MSIZE_16BIT);              // set MSIZE
    dma_set_peripheral_size(dma, dma_channel, DMA_CCR_PSIZE_16BIT);          // set PSIZE
    dma_enable_memory_increment_mode(dma, dma_channel);                      // set MINC
    dma_set_number_of_data(dma, dma_channel, buffer_size);            // set CNDTR
    dma_enable_circular_mode(dma, dma_channel);                              // set CIRC
    dma_set_priority(dma, dma_channel, DMA_CCR_PL_VERY_HIGH);
    dma_enable_channel(dma, dma_channel);                                    // set EN
}

void adc_initialize_periph(uint32_t rcc, uint32_t adc, uint8_t* channels, uint8_t channels_size)
{
    rcc_periph_clock_enable(rcc);

    adc_enable_temperature_sensor();
#if defined(STM32F3) || defined(STM32G4)
    adc_enable_regulator(adc);
      for (long i = 0; i < 100000; i++)
        asm("nop");
    // TODO should be adc argument
    adc_set_clk_prescale(ADC1, ADC_CCR_CKMODE_DIV1);
#endif
    adc_set_regular_sequence(adc, channels_size, channels);
    adc_calibrate(adc);

// TODO unify/de-duplicate libopencm3 here
// the reading the internal temperature sensor requires a minimum sampling time
#if defined(STM32F0)
    adc_set_sample_time_on_all_channels(adc, ADC_SMPTIME_071DOT5);
#else
    // TODO handle ADC overrun, longer sample times mitigate this for now.
    // todo increase dma channel priority
    adc_set_sample_time_on_all_channels(adc, ADC_SMPR_SMP_19DOT5CYC);
    // TODO restore/report temperature
    // adc_set_sample_time(ADC_PERIPH, ADC_CHANNEL_TEMPERATURE, ADC_SMPR_SMP_61DOT5CYC);
#endif

    adc_enable_dma(adc);
    adc_enable_dma_circular_mode(adc);
    adc_power_on(adc);
}

void adc_initialize()
{
#if defined(ADC_GPIOA_PINS)
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_GPIOA_PINS);
#endif

#if defined(ADC_GPIOB_PINS)
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_GPIOB_PINS);
#endif

#if defined(ADC_GPIOC_PINS)
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_GPIOC_PINS);
#endif

    adc_initialize_dma(RCC_DMA1, DMA1, DMA_CHANNEL1, (uint32_t)&g.adc_buffer, sizeof(g.adc_buffer));
#if defined(STM32G0) || defined(STM32G4)
    adc_initialize_dmamux(DMAMUX1, DMA_CHANNEL1, ADC_DMAMUX_REQID);
#endif
    adc_initialize_periph(ADC_RCC, ADC_PERIPH, adc_channels, sizeof(adc_channels));
}

void adc_start()
{
    adc_start_conversion_regular(ADC_PERIPH);
}

volatile uint16_t adc_read_channel(uint8_t channel)
{
    return g.adc_buffer[channel];
}
