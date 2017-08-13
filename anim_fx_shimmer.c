/*
 * Card's LED shimmering animation
 */

#include "anim_fx_shimmer.h"
#include "leds.h"
#include <prng.h>
#include <limits.h>
#include <stdbool.h>

void
anim_fx_shimmer_init(struct anim_fx_shimmer_state *state,
                     const uint8_t *led_list, size_t led_num,
                     struct anim_fx_shimmer_slot *slot_list, size_t slot_num,
                     uint8_t *prev_list, size_t prev_num,
                     uint8_t br,
                     unsigned int duration)
{
    size_t i;
    /* Brightness reduction of a fully-dimmed LED */
    uint8_t dim_br_off = br / 4;
    /* Number of dimming/restoring steps */
    uint8_t dim_step_num = 6;
    /* Brightness offset for each dimming/restoring step (absolute) */
    int8_t dim_step_br_off = (int8_t)dim_br_off / (int8_t)dim_step_num;
    if (dim_step_br_off == 0) {
        dim_step_br_off = 1;
        dim_step_num = dim_br_off;
    }
    /* Delay of each dimming/restoring step */
    const unsigned int dim_step_delay = 150 / dim_step_num;

    /* Initialize slot list */
    for (i = 0; i < slot_num; i++) {
        struct anim_fx_shimmer_slot *slot = &slot_list[i];
        struct anim_fx_shimmer_stage *stage_list = slot->stage_list;

        /* Fill in stage constants */
        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_BRIGHT].step_num = 1;
        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_BRIGHT].step_br_off = 0;

        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_DIMMING].step_num =
                                                        dim_step_num;
        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_DIMMING].step_br_off =
                                                        -dim_step_br_off;
        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_DIMMING].step_delay =
                                                        dim_step_delay;

        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_DIMMED].step_num = 1;
        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_DIMMED].step_br_off = 0;
        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_DIMMED].step_delay = 100;

        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_RESTORING].step_num =
                                                        dim_step_num;
        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_RESTORING].step_br_off =
                                                        dim_step_br_off;
        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_RESTORING].step_delay =
                                                        dim_step_delay;

        /* Position at the end of the cycle */
        slot->stage_idx = ANIM_FX_SHIMMER_STAGE_IDX_NUM - 1;
        slot->steps_left = 0;
        slot->delay_left = 0;

        /* Assume maximum brightness */
        slot->br = br;
    }

    /* Initialize previously-shimmered LED list */
    for (i = 0; i < prev_num; i++) {
        /* Invalid index means unused */
        prev_list[i] = LEDS_NUM;
    }

    /* Initialize state */
    state->led_list = led_list;
    state->led_num = led_num;
    state->br = br;
    state->slot_list = slot_list;
    state->slot_num = slot_num;
    state->prev_list = prev_list;
    state->prev_num = prev_num;
    state->prev_pos = 0;
    state->remaining = duration;
    state->delay = 0;
}

unsigned int
anim_fx_shimmer_step(struct anim_fx_shimmer_state *state)
{
    size_t i;
    struct anim_fx_shimmer_slot *slot;
    struct anim_fx_shimmer_stage *stage_list;
    unsigned int new_delay;

    /*
     * Advance the state of every dimmed ball
     * and determine delay to next update
     */
    new_delay = UINT_MAX;
    for (i = 0; i < state->slot_num; i++) {
        slot = &state->slot_list[i];
        stage_list = slot->stage_list;

        /* Subtract elapsed delay */
        slot->delay_left -= state->delay;

        /* While the current step has no delay left */
        while (slot->delay_left == 0) {
            /* While the current stage has no steps left */
            while (slot->steps_left == 0) {
                /* If cycle is over */
                if (slot->stage_idx >= (ANIM_FX_SHIMMER_STAGE_IDX_NUM - 1)) {
                    bool found;
                    size_t j;
                    /* Pick an LED that wasn't dimmed recently */
                    do {
                        found = false;
                        slot->led = state->led_list[((prng_next() & 0xffff) *
                                                     state->led_num) >> 16];
                        for (j = 0; j < state->prev_num && !found; j++) {
                            if (slot->led == state->prev_list[j]) {
                                found = true;
                            }
                        }
                    } while (found);
                    state->prev_list[state->prev_pos++] = slot->led;
                    if (state->prev_pos >= state->prev_num) {
                        state->prev_pos = 0;
                    }

                    /* Determine stage timing */
                    stage_list[ANIM_FX_SHIMMER_STAGE_IDX_BRIGHT].step_delay =
                            (prng_next() & 0xfff);

                    /* Restart */
                    slot->stage_idx = 0;
                } else {
                    /* Move onto next stage */
                    slot->stage_idx++;
                }
                slot->steps_left = stage_list[slot->stage_idx].step_num;
            }
            slot->steps_left--;
            slot->delay_left = stage_list[slot->stage_idx].step_delay;
            slot->br += stage_list[slot->stage_idx].step_br_off;
        }

        /* Determine delay to next update */
        if (slot->delay_left < new_delay) {
            new_delay = slot->delay_left;
        }
    }

    /*
     * Schedule LED updates of all the balls changing next
     */
    for (i = 0; i < state->slot_num; i++) {
        slot = &state->slot_list[i];
        /* If the ball is changing on the next step */
        if (slot->delay_left == new_delay) {
            /* Schedule brightness update */
            LEDS_BR[slot->led] = slot->br;
        }
    }

    /* Call us after the next update */
    return (state->delay = new_delay);
}

