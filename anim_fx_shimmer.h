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

/** Slot for a shimmering LED */
struct anim_fx_shimmer_slot {
    /** LED index */
    uint8_t                         led;
    /** Stages */
    struct anim_fx_shimmer_stage    stage_list[ANIM_FX_SHIMMER_STAGE_IDX_NUM];
    /** Current stage index */
    enum anim_fx_shimmer_stage_idx  stage_idx;
    /** Current stage's remaining steps */
    uint8_t                         steps_left;
    /** Current step's remaining delay */
    unsigned int                    delay_left;
    /** Current brightness */
    int8_t                          br;
};

/** State of shimmering animation */
struct anim_fx_shimmer_state {
    /** Array of LED indices to shimmer */
    const uint8_t                  *led_list;
    /** Number of LED indices to shimmer */
    size_t                          led_num;

    /** "Bright" LED brightness */
    uint8_t                         br;

    /** Array of slots for currently shimmering LEDs */
    struct anim_fx_shimmer_slot    *slot_list;
    /** Number of slots for currently shimmering LEDs */
    size_t                          slot_num;

    /** Circular array of LED indices which were shimmering last */
    uint8_t                        *prev_list;
    /** Size of circular array of LED indices which were shimmering last */
    size_t                          prev_num;
    /**
     * Position where the next LED index should be written in circular array
     * of LEDs which shimmered last
     */
    size_t                          prev_pos;

    /**
     * Remaining effect time, ms. UINT_MAX means infinity.
     */
    unsigned int                    remaining;

    /**
     * Delay until the last step should take effect,
     * since the previous one did, ms.
     */
    unsigned int                    delay;
};

/**
 * Initialize an LED shimmering state.
 *
 * @param state         The state to initialize.
 * @param led_list      Array of indices of LEDs to shimmer.
 * @param led_num       Number of indices of LEDs to shimmer.
 * @param slot_list     Array of LED shimmering slots.
 * @param slot_num      Number of LED shimmering slots.
 * @param prev_list     Circular array of LED indices which were shimmering last.
 * @param prev_num      Size of circular array of LED indices which were shimmering last.
 * @param br            Nominal brightness of non-dimmed LEDs.
 * @param duration      Duration of shimmering, excluding fade-in/out time,
 *                      UINT_MAX means infinity (and no fade-out).
 *
 */
extern void anim_fx_shimmer_init(struct anim_fx_shimmer_state *state,
                                 const uint8_t *led_list,
                                 size_t led_num,
                                 struct anim_fx_shimmer_slot *slot_list,
                                 size_t slot_num,
                                 uint8_t *prev_list,
                                 size_t prev_num,
                                 uint8_t br,
                                 unsigned int duration);

/**
 * Execute an LED shimmering step with a specified state.
 *
 * @return The delay after which the state updated by this function should
 *         become active.
 */
extern unsigned int anim_fx_shimmer_step(struct anim_fx_shimmer_state *state);

#endif /* _ANIM_FX_SHIMMER_H */
