/*
 * LEDs control
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
#include <misc.h>
#include <stdbool.h>

/** SPI peripheral to use to talk to LEDs */
static volatile struct spi *LEDS_SPI;

/** The GPIO peripheral controlling the load-enable (LE) pin */
static volatile struct gpio *LEDS_LE_GPIO;

/** The GPIO pin controlling the load-enable (LE) pin */
static unsigned int LEDS_LE_PIN;

/** Brightness value to pulse length map */
static const uint8_t LEDS_BR_PL[LEDS_BR_NUM] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04,
    0x04, 0x04, 0x05, 0x05, 0x05, 0x06, 0x06, 0x07,
    0x07, 0x08, 0x09, 0x09, 0x0a, 0x0b, 0x0b, 0x0c,
    0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x13, 0x14, 0x16,
    0x17, 0x19, 0x1a, 0x1c, 0x1e, 0x21, 0x23, 0x25,
    0x28, 0x2b, 0x2e, 0x31, 0x34, 0x38, 0x3c, 0x40
};

/** Brightness value of each LED */
uint8_t LEDS_BR[LEDS_NUM];

/** State of each LED for each PWM step, in two banks */
static volatile uint8_t LEDS_PWM_BANKS[2][LEDS_BR_NUM][LEDS_NUM / 8];

/** Index of the PWM LED state bank currently being output */
static volatile size_t LEDS_PWM_BANK;

const uint8_t LEDS_STARS_LIST[LEDS_STARS_NUM] = {
    19, 17, 16, 27, 18, 26, 25, 31, 15,
    20, 24, 29, 30, 21, 28, 22, 7, 23
};

const uint8_t LEDS_BALLS_LIST[LEDS_BALLS_NUM] = {
    6, 33, 13, 5, 34, 14, 12, 4, 3, 36,
    11, 10, 37, 2, 38, 9, 39, 0, 35, 8, 1
};

const uint8_t LEDS_BALLS_SWNE_LINE_LIST[LEDS_BALLS_SWNE_LINE_NUM]
                                       [LEDS_BALLS_SWNE_LINE_LEN] = {
#define LINE(_leds...) {_leds, LEDS_IDX_INVALID}
    LINE(33, 6),
    LINE(5, 13),
    LINE(3, 12, 34),
    LINE(38, 10, 36, 4, 14),
    LINE(0, 9, 2, 37, 11),
    LINE(35, 8, 39),
    LINE(1),
#undef LINE
};

void
leds_init(volatile struct spi *spi,
          volatile struct gpio *le_gpio,
          unsigned int le_pin)
{
    size_t bank;
    uint8_t step;
    size_t i;

    /* Store params */
    LEDS_SPI = spi;
    LEDS_LE_GPIO = le_gpio;
    LEDS_LE_PIN = le_pin;

    /* Zero LED brightness */
    for (i = 0; i < ARRAY_SIZE(LEDS_BR); i++) {
        LEDS_BR[i] = 0;
    }

    /* Zero all PWM banks */
    for (bank = 0; bank < ARRAY_SIZE(LEDS_PWM_BANKS); bank++) {
        for (step = 0; step < ARRAY_SIZE(LEDS_PWM_BANKS[bank]); step++) {
            for (i = 0; i < ARRAY_SIZE(LEDS_PWM_BANKS[bank][step]); i++) {
                LEDS_PWM_BANKS[bank][step][i] = 0;
            }
        }
    }

    /* Set bank 0 active */
    LEDS_PWM_BANK = 0;
}

void
leds_render(void)
{
    /* Use inactive bank */
    size_t bank = !LEDS_PWM_BANK;
    uint8_t step;
    size_t i;

    /* For each PWM step */
    for (step = 0; step < ARRAY_SIZE(LEDS_PWM_BANKS[bank]); step++) {
        /* Zero the step */
        for (i = 0; i < ARRAY_SIZE(LEDS_PWM_BANKS[bank][step]); i++) {
            LEDS_PWM_BANKS[bank][step][i] = 0;
        }
        /* Render the step */
        for (i = 0; i < (ARRAY_SIZE(LEDS_PWM_BANKS[bank][step]) << 3); i++) {
            LEDS_PWM_BANKS[bank][step][i >> 3] |=
                (LEDS_BR_PL[LEDS_BR[i]] > step) << (i & 0x7);
        }
    }
}

void
leds_swap(void)
{
    LEDS_PWM_BANK = !LEDS_PWM_BANK;
}

void
leds_step_send(size_t step)
{
    /* Use active bank */
    size_t bank = LEDS_PWM_BANK;
    size_t i;

    /* Disable loading the data to the outputs */
    gpio_pin_set(LEDS_LE_GPIO, LEDS_LE_PIN, false);

    /* For each LED state byte */
    for (i = 0; i < ARRAY_SIZE(LEDS_PWM_BANKS[bank][step]); i++) {
        /* Receive and discard the last answer, if any */
        if (LEDS_SPI->sr & SPI_SR_RXNE_MASK) {
            unsigned int discard = LEDS_SPI->dr;
            (void)discard;
        }
        /* Wait for transmit register to be empty */
        while (!(LEDS_SPI->sr & SPI_SR_TXE_MASK));
        /* Output the state byte */
        LEDS_SPI->dr = LEDS_PWM_BANKS[bank][step][i];
    }
}

void
leds_step_load(void)
{
    /* Enable loading the data to the outputs */
    gpio_pin_set(LEDS_LE_GPIO, LEDS_LE_PIN, true);
}
