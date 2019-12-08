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

    rcc_periph_clock_enable(RCC_DMA1);
    dma_set_peripheral_address(DMA1, DMA_CHANNEL1, (uint32_t)&ADC_DR(ADC_PERIPH)); // set CPAR
    dma_set_memory_address(DMA1, DMA_CHANNEL1, (uint32_t)&g.adc_buffer);             // set CMAR
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL1);                              // set DIR
    dma_set_memory_size(DMA1, DMA_CHANNEL1, DMA_CCR_MSIZE_16BIT);              // set MSIZE
    dma_set_peripheral_size(DMA1, DMA_CHANNEL1, DMA_CCR_PSIZE_16BIT);          // set PSIZE
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL1);                      // set MINC
    dma_set_number_of_data(DMA1, DMA_CHANNEL1, sizeof(adc_channels));            // set CNDTR
    dma_enable_circular_mode(DMA1, DMA_CHANNEL1);                              // set CIRC
    dma_set_priority(DMA1, DMA_CHANNEL1, DMA_CCR_PL_VERY_HIGH);
    dma_enable_channel(DMA1, DMA_CHANNEL1);                                    // set EN

#if defined(STM32G0) || defined(STM32G4)
  DMAMUX_CxCR(DMAMUX1, DMA_CHANNEL1) = ADC_DMAMUX_REQID;
#endif

    rcc_periph_clock_enable(ADC_RCC);

    adc_enable_temperature_sensor();
#if defined(STM32F3) || defined(STM32G4)
    adc_enable_regulator(ADC_PERIPH);
      for (long i = 0; i < 100000; i++)
        asm("nop");
    // TODO should be ADC_PERIPH
    adc_set_clk_prescale(ADC1, ADC_CCR_CKMODE_DIV1);
#endif
    adc_set_regular_sequence(ADC_PERIPH, sizeof(adc_channels), adc_channels);
    adc_calibrate(ADC_PERIPH);

// TODO unify/de-duplicate libopencm3 here
// the reading the internal temperature sensor requires a minimum sampling time
#if defined(STM32F0)
    adc_set_sample_time_on_all_channels(ADC_PERIPH, ADC_SMPTIME_071DOT5);
#else
    // TODO handle ADC overrun, longer sample times mitigate this for now.
    // todo increase dma channel priority
    adc_set_sample_time_on_all_channels(ADC_PERIPH, ADC_SMPR_SMP_4DOT5CYC);
    adc_set_sample_time(ADC_PERIPH, ADC_CHANNEL_TEMPERATURE, ADC_SMPR_SMP_61DOT5CYC);
#endif

    adc_enable_dma(ADC_PERIPH);
    adc_enable_dma_circular_mode(ADC_PERIPH);
    adc_power_on(ADC_PERIPH);
}

void adc_start()
{
    adc_start_conversion_regular(ADC_PERIPH);
}

volatile uint16_t adc_read_channel(uint8_t channel)
{
    return g.adc_buffer[channel];
}
