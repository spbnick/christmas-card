/*
 * Card's LED scripted animation.
 *
 * The effect starts with fade-in and ends with fade-out,
 * both while animating at the same time.
 */

#ifndef _ANIM_FX_SCRIPT_H
#define _ANIM_FX_SCRIPT_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/** Script segment description */
struct anim_fx_script_seg {
    /** Minimum number of steps */
    uint8_t         step_num_min;
    /** Maximum number of steps */
    uint8_t         step_num_max;
    /** Brightness offset of each step */
    int8_t          step_br_off;
    /** Minimum step delay */
    unsigned int    step_delay_min;
    /** Maximum step delay */
    unsigned int    step_delay_max;
};

/**
 * Per-LED state of a script segment.
 * Updated each time a LED goes through all its segments.
 */
struct anim_fx_script_led_seg {
    /** Number of steps */
    uint8_t         step_num;
    /** Delay of each step */
    unsigned int    step_delay;
};

/** State of an animated LED */
struct anim_fx_script_led {
    /** LED index */
    uint8_t                                 idx;
    /** List of segment states */
    struct anim_fx_script_led_seg          *seg_list;
    /** Current segment index */
    uint8_t                                 seg_idx;
    /** Current segment's remaining steps */
    uint8_t                                 steps_left;
    /** Current step's remaining delay */
    unsigned int                            delay_left;
    /** Current brightness */
    uint8_t                                 br;
    /** Next brightness */
    uint8_t                                 next_br;
};

/** State of a scripted animation */
struct anim_fx_script {
    /** Number of segments */
    uint8_t                             seg_num;
    /** List of segment descriptions [seg_num] */
    const struct anim_fx_script_seg    *seg_list;

    /** Number of LEDs */
    uint8_t                             led_num;
    /** Array of LED states [led_num] */
    struct anim_fx_script_led          *led_list;

    /** Number of fade-in/out steps */
    uint8_t                             fade_step_num;
    /** Delay (duration) of each fade step, ms */
    unsigned int                        fade_step_delay;

    /** Number of fade-in/out steps left */
    uint8_t                             fade_steps_left;
    /** Delay left in current fade-in/out step, ms */
    unsigned int                        fade_step_delay_left;

    /** Animation duration (excluding fade-in/out), ms **/
    unsigned int                        duration;

    /**
     * Delay until the last step should take effect,
     * since the previous one did, ms.
     */
    unsigned int                        delay;
};

/**
 * Initialize an LED script state.
 *
 * @param script            The scripted animation state to initialize.
 * @param seg_num           Number of script segments to animate through.
 *                          Length of seg_list and elements of
 *                          led_seg_list_list.
 * @param seg_list          List of script segments to animate through
 *                          [seg_num].
 * @param led_num           Number of LEDs to animate.  Length of idx_list,
 *                          led_list, and led_seg_list_list.
 * @param idx_list          Array of indices of LEDs to animate [led_num].
 * @param led_list          Array of animated LED states [led_num].
 * @param led_seg_list_list List of segments states for each LED
 *                          [led_num * seg_num].
 * @param br                Initial LED brightness.
 * @param fade_delay        Fade-in/out delay, ms.
 * @param duration          Animation duration (excluding fade-in/out), ms.
 *                          UINT_MAX for infinity (no fade-out).
 */
extern void anim_fx_script_init(
                            struct anim_fx_script *script,
                            uint8_t seg_num,
                            const struct anim_fx_script_seg *seg_list,
                            uint8_t led_num,
                            const uint8_t *idx_list,
                            struct anim_fx_script_led *led_list,
                            struct anim_fx_script_led_seg *led_seg_list_list,
                            uint8_t br,
                            unsigned int fade_delay,
                            unsigned int duration);

/**
 * Execute an scripted step with a specified state.
 *
 * @param pdelay    Location for the delay after which the state updated by
 *                  this function should become active.
 *
 * @return True if the function scheduled the last step, false otherwise.
 */
extern bool anim_fx_script_step(struct anim_fx_script *state,
                                 unsigned int *pdelay);

#endif /* _ANIM_FX_SCRIPT_H */
