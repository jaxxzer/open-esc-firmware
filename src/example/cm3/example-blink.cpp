#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

int main()
{
rcc_periph_clock_enable(RCC_GPIOA);

/* Set GPIO12 (in GPIO port D) to 'output push-pull'. */
gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT,
        GPIO_PUPD_NONE, GPIO5);


while(1) {
        gpio_toggle(GPIOA, GPIO5);
        for(uint32_t i = 0; i < 100000; i++) {
                int a = 22 * 5;
                asm("nop");
                asm("nop");
                asm("nop");
                asm("nop");
                asm("nop");
                asm("nop");
                asm("nop");
                asm("nop");
                asm("nop");
                asm("nop");
        }
}
}
