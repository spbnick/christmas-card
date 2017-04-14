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

/** Invalid LED index */
#define LEDS_IDX_INVALID    255

/** Number of LED brightness values */
#define LEDS_BR_NUM     64

/** Maximum LED brightness value */
#define LEDS_BR_MAX     (LEDS_BR_NUM - 1)

/** Brightness value of each LED */
extern uint8_t LEDS_BR[LEDS_NUM];

/** Number of star LEDs */
#define LEDS_STARS_NUM  18

/** List of indexes of star LEDs, left-to-right, top-to-bottom */
extern const uint8_t LEDS_STARS_LIST[LEDS_STARS_NUM];

/** Number of treetopper LEDs */
#define LEDS_TOPPER_NUM 1

/** List of indexes of treetopper LEDs, left-to-right, top-to-bottom */
extern const uint8_t LEDS_TOPPER_LIST[LEDS_TOPPER_NUM];

/** Number of ball LEDs */
#define LEDS_BALLS_NUM  21

/** List of indexes of ball LEDs, left-to-right, top-to-bottom */
extern const uint8_t LEDS_BALLS_LIST[LEDS_BALLS_NUM];

/**
 * Maximum length of a southwest-northeast diagonal line of ball LEDs,
 * including terminating invalid index
 */
#define LEDS_BALLS_SWNE_LINE_LEN  6

/** Number of southwest-northeast diagonal lines of ball LEDs */
#define LEDS_BALLS_SWNE_LINE_NUM  7

/**
 * List of southwest-northeast diagonal lines of ball LEDs, top-to-bottom.
 * Each line contains LED indexes terminated by the invalid LED index.
 */
extern const uint8_t LEDS_BALLS_SWNE_LINE_LIST[LEDS_BALLS_SWNE_LINE_NUM]
                                              [LEDS_BALLS_SWNE_LINE_LEN];

/**
 * Initialize LEDs module.
 *
 * @param spi       The SPI peripheral to use to write to LEDs.
 * @param le_gpio   The GPIO peripheral controlling the load-enable (LE)
 *                  signal.
 * @param le_pin    The GPIO pin controlling the load-enable (LE) signal.
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
