/*
 * LEDs control
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

    /* Set bank 0 active */
    LEDS_PWM_BANK = 0;

    /* For each PWM step */
    bank = LEDS_PWM_BANK;
    for (step = 0; step < ARRAY_SIZE(LEDS_PWM_BANKS[bank]); step++) {
        /* Zero the step */
        for (i = 0; i < ARRAY_SIZE(LEDS_PWM_BANKS[bank][step]); i++) {
            LEDS_PWM_BANKS[bank][step][i] = 0;
        }
    }
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
