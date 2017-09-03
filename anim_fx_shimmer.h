/*
 * Card's LED shimmering animation.
 *
 * The effect starts with fade-in and ends with fade-out, both 2 seconds long,
 * and both while shimmering at the same time.
 */

#ifndef _ANIM_FX_SHIMMER_H
#define _ANIM_FX_SHIMMER_H

#include <stdlib.h>
#include <stdint.h>

/** Shimmer stage description */
struct anim_fx_shimmer_stage {
    /** Number of steps */
    uint8_t         step_num;
    /** Brightness offset of each step */
    int8_t          step_br_off;
    /** Delay of each step */
    unsigned int    step_delay;
};

/** Indexes of possible stages of shimmering LEDs */
enum anim_fx_shimmer_stage_idx {
    /** Waiting to be dimmed */
    ANIM_FX_SHIMMER_STAGE_IDX_BRIGHT,
    /** Dimming */
    ANIM_FX_SHIMMER_STAGE_IDX_DIMMING,
    /** Dimmed */
    ANIM_FX_SHIMMER_STAGE_IDX_DIMMED,
    /** Restoring */
    ANIM_FX_SHIMMER_STAGE_IDX_RESTORING,
    /** Number of stage IDs, not a valid ID */
    ANIM_FX_SHIMMER_STAGE_IDX_NUM
};

/** State of a shimmering LED */
struct anim_fx_shimmer_led {
    /** LED index */
    uint8_t                         idx;
    /** Stages */
    struct anim_fx_shimmer_stage    stage_list[ANIM_FX_SHIMMER_STAGE_IDX_NUM];
    /** Current stage index */
    enum anim_fx_shimmer_stage_idx  stage_idx;
    /** Current stage's remaining steps */
    uint8_t                         steps_left;
    /** Current step's remaining delay */
    unsigned int                    delay_left;
    /** Current brightness */
    uint8_t                         br;
    /** Next brightness */
    uint8_t                         next_br;
};

/** State of shimmering animation */
struct anim_fx_shimmer {
    /** "Bright" LED brightness */
    uint8_t                     br;
    /** Maximum "bright" state delay, ms */
    unsigned int                br_delay;
    /** Maximum "dimmed" state delay, ms */
    unsigned int                dim_delay;

    /** Array of LED states */
    struct anim_fx_shimmer_led *led_list;
    /** Number of LEDs */
    uint8_t                     led_num;

    /**
     * Delay until the last step should take effect,
     * since the previous one did, ms.
     */
    unsigned int                delay;
};

/**
 * Initialize an LED shimmering state.
 *
 * @param shimmer   The shimmering animation state to initialize.
 * @param led_list  Array of shimmering LED states.
 * @param idx_list  Array of indices of LEDs to shimmer.
 * @param led_num   Number of LEDs to shimmer.
 *                  Length of both idx_list and led_list.
 * @param br        Nominal brightness of non-dimmed LEDs.
 * @param br_delay  Maximum bright state delay, ms.
 * @param dim_delay Maximum dimmed state delay, ms.
 */
extern void anim_fx_shimmer_init(struct anim_fx_shimmer *shimmer,
                                 struct anim_fx_shimmer_led *led_list,
                                 const uint8_t *idx_list,
                                 size_t led_num,
                                 uint8_t br,
                                 unsigned int br_delay,
                                 unsigned int dim_delay);

/**
 * Execute an LED shimmering step with a specified state.
 *
 * @return The delay after which the state updated by this function should
 *         become active.
 */
extern unsigned int anim_fx_shimmer_step(struct anim_fx_shimmer *state);

#endif /* _ANIM_FX_SHIMMER_H */
