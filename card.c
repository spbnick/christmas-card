/*
 * A simple blinking Christmas card firmware
 *
 * A4 - GPIO    - LE(ED1)
 * A5 - SCK     - CLK
 * A6 - MISO    - SDO
 * A7 - MOSI    - SDI
 */
#include "anim.h"
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

/* SPI peripheral to use to talk to LEDs */
static volatile struct spi *SPI;

/* Systick handler step */
static volatile unsigned int SYSTICK_STEP;
/* True if systick handler must swap LED banks */
static volatile bool SYSTICK_SWAP_WAIT;
/*
 * Value of SYSTICK_STEP at (or after) which systick handler must swap LED
 * banks. Rounded to PWM step zero.
 */
static volatile unsigned int SYSTICK_SWAP_NEXT;
/* Last value of SYSTICK_STEP at which the LED banks were swapped */
static volatile unsigned int SYSTICK_SWAP_LAST;

/* The maxmimum lag for swap step time to be considered not overrun */
#define SYSTICK_SWAP_LAG    ((unsigned int)1 << 31)

/** Systick handler */
void systick_handler(void) __attribute__ ((isr));
void
systick_handler(void)
{
    /* Current tick value */
    unsigned int step = SYSTICK_STEP;
    unsigned int pwm_step = (step >> 1) & LEDS_BR_MAX;

    if (step % 24000 == 0) {
        GPIO_C->odr ^= GPIO_ODR_ODR13_MASK;
    }

    /* If it's the odd tick */
    if (step & 1) {
        leds_step_load();
    } else {
        /*
         * If we're on the new PWM cycle, and we are asked to swap the LED PWM
         * data banks, and the time has arrived (accounting for rollover).
         */
        if (pwm_step == 0 &&
            SYSTICK_SWAP_WAIT &&
            SYSTICK_SWAP_NEXT <= SYSTICK_STEP &&
            (SYSTICK_STEP - SYSTICK_SWAP_NEXT < SYSTICK_SWAP_LAG)) {
            /* Swap the LED banks */
            leds_swap();
            SYSTICK_SWAP_LAST = step;
            SYSTICK_SWAP_WAIT = false;
        }
        leds_step_send(pwm_step);
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

    /* Initialize LED states */
    leds_init(SPI, GPIO_A, 4);

    /* Initialize animation state */
    anim_init();

    /* Initialize the systick handler */
    SYSTICK_STEP = 0;
    SYSTICK_SWAP_WAIT = false;
    SYSTICK_SWAP_LAST = 0;
    SYSTICK_SWAP_NEXT = 0;

    /*
     * Set SysTick timer to fire the interrupt at frequency 375 * 64 * 2 =
     * 48KHz, setting the unit to HCLK (72MHz). This way we can have PWM
     * frequency of 375 Hz, 64 pulse lengths, and also trigger Load-Enable
     * every other pulse.
     */
    STK->val = STK->load = 72000000 / 375 / 64 / 2 - 1;
    STK->ctrl |= STK_CTRL_ENABLE_MASK | STK_CTRL_TICKINT_MASK |
                 (STK_CTRL_CLKSOURCE_VAL_AHB << STK_CTRL_CLKSOURCE_LSB);

    {
        unsigned int delay;
        while (true) {
            while (SYSTICK_SWAP_WAIT) {
                asm ("wfi");
            }
            delay = anim_step();
            leds_render();
            SYSTICK_SWAP_NEXT = SYSTICK_SWAP_LAST + delay * 48;
            SYSTICK_SWAP_WAIT = true;
        }
    }
}
