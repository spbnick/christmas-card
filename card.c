/*
 * A simple blinking Christmas card firmware
 *
 * A4 - GPIO    - LE(ED1)
 * A5 - SCK     - CLK
 * A6 - MISO    - SDO
 * A7 - MOSI    - SDI
 *
 *
 * Rough map of the LEDs with bit numbers
 *
 *  19W                                        16W
 *                     17W        / \                       27W
 *                              < 32Y >
 *                               |/ \|
 *          18W
 *                                                  26W
 *                                   06R                     25W
 *                  15W
 *       31W
 *                            33Y                      20W
 *                                                                24W
 *               30W                     13G
 *                                                   21W
 *     29W                  05R
 *
 *
 *                                    34Y                       22W
 *          28W                                14G
 *                           12G
 *                                         04R
 *    07W               03R                                 23W
 *
 *                                  36Y           11G
 *
 *                           10G             37Y
 *
 *                                     02R
 *                    38Y        09G             39Y
 *
 *                                         08G
 *                   00R        35Y                   01R
 *
 */
#include "leds.h"
#include <rcc.h>
#include <gpio.h>
#include <init.h>
#include <tim.h>
#include <spi.h>
#include <stk.h>
#include <misc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define STOP \
    do {                \
        asm ("wfi");    \
    } while (1)

/* SPI peripheral to use to talk to LEDs */
static volatile struct spi *SPI;

/* Systick handler step */
static volatile unsigned int SYSTICK_STEP;

/** Systick handler */
void systick_handler(void) __attribute__ ((isr));
void
systick_handler(void)
{
    /* Current tick value */
    unsigned int tick = SYSTICK_STEP;

    if (tick % 24000 == 0) {
        GPIO_C->odr ^= GPIO_ODR_ODR13_MASK;
    }

    /* If it's the odd tick */
    if (tick & 1) {
        leds_step_load();
    } else {
        leds_step_send((tick >> 1) & LEDS_BR_MAX);
    }

    SYSTICK_STEP++;
}

void
reset(void)
{
    /* Basic init */
    init();

    /* Use the first SPI peripheral */
    SPI = SPI1;

    /*
     * Enable clocks
     */
    /* Enable APB2 clock to I/O port A and SPI1 */
    RCC->apb2enr |= RCC_APB2ENR_IOPAEN_MASK | RCC_APB2ENR_IOPCEN_MASK | RCC_APB2ENR_SPI1EN_MASK;

    /*
     * Configure pins
     */
    /* A4 - GPIO - LE(ED1), push-pull output */
    gpio_pin_set(GPIO_A, 4, false);
    gpio_pin_conf(GPIO_A, 4,
                  GPIO_MODE_OUTPUT_2MHZ, GPIO_CNF_OUTPUT_GP_PUSH_PULL);
    /* A5 - SCK, alternate function push-pull */
    gpio_pin_conf(GPIO_A, 5,
                  GPIO_MODE_OUTPUT_2MHZ, GPIO_CNF_OUTPUT_AF_PUSH_PULL);
    /* A6 - MISO, input pull-up */
    gpio_pin_conf(GPIO_A, 6,
                  GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL);
    gpio_pin_set(GPIO_A, 6, true);
    /* A7 - MOSI, alternate function push-pull */
    gpio_pin_conf(GPIO_A, 7,
                  GPIO_MODE_OUTPUT_2MHZ, GPIO_CNF_OUTPUT_AF_PUSH_PULL);
    /* Set PC13 to general purpose open-drain output, max speed 2MHz */
    gpio_pin_conf(GPIO_C, 13,
                  GPIO_MODE_OUTPUT_2MHZ, GPIO_CNF_OUTPUT_GP_OPEN_DRAIN);

    /*
     * Configure the SPI
     * Set it to run at APB2clk/8, make it a master, enable software NSS pin
     * management, raise it, and enable SPI.
     */
    SPI->cr1 = (SPI->cr1 &
                ~(SPI_CR1_BR_MASK | SPI_CR1_MSTR_MASK | SPI_CR1_SPE_MASK |
                  SPI_CR1_SSM_MASK | SPI_CR1_SSI_MASK)) |
               (SPI_CR1_BR_VAL_FPCLK_DIV8 << SPI_CR1_BR_LSB) |
               (SPI_CR1_MSTR_VAL_MASTER << SPI_CR1_MSTR_LSB) |
               SPI_CR1_SSM_MASK | SPI_CR1_SSI_MASK | SPI_CR1_SPE_MASK;

    /*
     * Initialize LED states
     */
    {
        leds_init(SPI, GPIO_A, 4);
        size_t i;
        for (i = 0; i < ARRAY_SIZE(LEDS_BR); i++) {
            LEDS_BR[i] = LEDS_BR_MAX / 3;
        }
        leds_render();
        leds_swap();
    }

    /* Initialize the systick handler */
    SYSTICK_STEP = 0;

    /*
     * Set SysTick timer to fire the interrupt at frequency 375 * 64 * 2 =
     * 48KHz, setting the unit to HCLK (72MHz). This way we can have PWM
     * frequency of 375 Hz, 64 pulse lengths, and also trigger Load-Enable
     * every other pulse.
     */
    STK->val = STK->load = 72000000 / 375 / 64 / 2;
    STK->ctrl |= STK_CTRL_ENABLE_MASK | STK_CTRL_TICKINT_MASK |
                 (STK_CTRL_CLKSOURCE_VAL_AHB << STK_CTRL_CLKSOURCE_LSB);

    STOP;
}
