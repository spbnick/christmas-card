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
 * Render current brightness of each LED into the inactive PWM data bank.
 */
extern void leds_render(void);

/**
 * Swap the active and inactive PWM data banks.
 */
extern void leds_swap(void);

/**
 * Send the specified LED state step of the active PWM data bank.
 *
 * @param step  The step to output. Must be <= LEDS_BR_MAX.
 */
extern void leds_step_send(size_t step);

/**
 * Load the last sent LED state.
 */
extern void leds_step_load(void);

#endif /* _LEDS_H */
