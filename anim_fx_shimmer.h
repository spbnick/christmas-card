/*
 * Card's LED shimmering animation script setup.
 */

#ifndef _ANIM_FX_SHIMMER_H
#define _ANIM_FX_SHIMMER_H

#include "anim_fx_script.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/** Indexes of possible stages of shimmering LEDs */
enum anim_fx_shimmer_seg_idx {
    /** Waiting to be dimmed */
    ANIM_FX_SHIMMER_SEG_IDX_BRIGHT,
    /** Dimming */
    ANIM_FX_SHIMMER_SEG_IDX_DIMMING,
    /** Dimmed */
    ANIM_FX_SHIMMER_SEG_IDX_DIMMED,
    /** Restoring */
    ANIM_FX_SHIMMER_SEG_IDX_RESTORING,
    /** Number of stage indexes, not a valid index */
    ANIM_FX_SHIMMER_SEG_IDX_NUM
};

/**
 * Initialize an LED shimmering script state.
 *
 * @param script            The scripted animation state to initialize.
 * @param seg_list          List of script segments to animate through
 *                          [ANIM_FX_SHIMMER_SEG_IDX_NUM].
 * @param led_num           Number of LEDs to animate.  Length of idx_list,
 *                          led_list, and led_seg_list_list.
 * @param idx_list          Array of indices of LEDs to animate [led_num].
 * @param led_list          Array of animated LED states [led_num].
 * @param led_seg_list_list List of segments states for each LED
 *                          [led_num * ANIM_FX_SHIMMER_SEG_IDX_NUM].
 * @param bright_br         Brightness of non-dimmed LEDs.
 * @param bright_delay      Maximum bright state delay, ms.
 * @param dimmed_br         Brightness of dimmed LEDs.
 * @param dimmed_delay      Maximum dimmed state delay, ms.
 * @param fade_delay        Fade-in/out delay, ms.
 * @param duration          Animation duration (excluding fade-in/out), ms.
 *                          UINT_MAX for infinity (no fade-out).
 */
extern void anim_fx_shimmer_script_init(
                            struct anim_fx_script *script,
                            struct anim_fx_script_seg *seg_list,
                            uint8_t led_num,
                            const uint8_t *idx_list,
                            struct anim_fx_script_led *led_list,
                            struct anim_fx_script_led_seg *led_seg_list_list,
                            uint8_t bright_br,
                            unsigned int bright_delay,
                            uint8_t dimmed_br,
                            unsigned int dimmed_delay,
                            unsigned int fade_delay,
                            unsigned int duration);

#endif /* _ANIM_FX_SHIMMER_H */
