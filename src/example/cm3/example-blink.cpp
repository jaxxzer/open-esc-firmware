#include <target.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>


int main()
{
rcc_periph_clock_enable(GPIO_LED1_RCC);

/* Set GPIO12 (in GPIO port D) to 'output push-pull'. */
gpio_mode_setup(GPIO_LED1_PORT, GPIO_MODE_OUTPUT,
        GPIO_PUPD_NONE, GPIO_LED1_PIN);


while(1) {
        gpio_toggle(GPIO_LED1_PORT, GPIO_LED1_PIN);
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
