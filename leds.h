/*
 * LEDs control
 */

#ifndef _LEDS_H
#define _LEDS_H

#include <spi.h>
#include <gpio.h>
#include <stddef.h>
#include <stdint.h>

/** Number of LEDs */
#define LEDS_NUM        40

/** Number of LED brightness values */
#define LEDS_BR_NUM     (64)

/** Maximum LED brightness value */
#define LEDS_BR_MAX     (LEDS_BR_NUM - 1)

/** Brightness value of each LED */
extern uint8_t LEDS_BR[LEDS_NUM];

/**
 * Initialize LEDs module.
 *
 * @param spi       The SPI peripheral to use to write to LEDs.
 * @param le_gpio   The GPIO peripheral controlling the load-enable (LE) pin.
 * @param le_pin    The GPIO pin controlling the load-enable (LE) pin.
 */
extern void leds_init(volatile struct spi *spi,
                      volatile struct gpio *le_gpio,
                      unsigned int le_pin);

/**
 * Render current brightness of each LED into PWM data and make it available
 * for sending.
 */
extern void leds_render(void);

/**
 * Send the LED state for the specified PWM step
 *
 * @param step  The step to output. Must be <= LEDS_BR_MAX.
 */
extern void leds_step_send(size_t step);

/**
 * Load the last output LED state for a PWM step.
 */
extern void leds_step_load(void);

#endif /* _LEDS_H */
