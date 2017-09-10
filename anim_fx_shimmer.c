/*
 * Card's LED shimmering animation
 */

#include "anim_fx_shimmer.h"
#include "leds.h"
#include <prng.h>
#include <misc.h>
#include <limits.h>

void
anim_fx_shimmer_init(struct anim_fx_shimmer *shimmer,
                     struct anim_fx_shimmer_led *led_list,
                     const uint8_t *idx_list,
                     size_t led_num,
                     uint8_t bright_br,
                     unsigned int bright_delay,
                     uint8_t dimmed_br,
                     unsigned int dimmed_delay,
                     unsigned int fade_delay,
                     unsigned int duration)
{
    size_t i;
    /* Number of LEDs dimmed at once */
    size_t dim_num = led_num / 4;
    if (dim_num == 0) {
        dim_num = led_num / 2;
    }
    if (dim_num == 0) {
        dim_num = led_num;
    }
    /* Brightness reduction of a fully-dimmed LED */
    uint8_t dim_br_off = bright_br - dimmed_br;
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

    /* Initialize led list */
    for (i = 0; i < led_num; i++) {
        struct anim_fx_shimmer_led *led = &led_list[i];
        struct anim_fx_shimmer_stage *stage_list = led->stage_list;

        /* Assign the LED */
        led->idx = idx_list[i];

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

        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_RESTORING].step_num =
                                                        dim_step_num;
        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_RESTORING].step_br_off =
                                                        dim_step_br_off;
        stage_list[ANIM_FX_SHIMMER_STAGE_IDX_RESTORING].step_delay =
                                                        dim_step_delay;

        /* Position at the end of the cycle */
        led->stage_idx = ANIM_FX_SHIMMER_STAGE_IDX_NUM - 1;
        led->steps_left = 0;
        led->delay_left = 0;

        /* Assume bright */
        led->br = bright_br;
    }

    /* Initialize state */
    shimmer->bright_delay = bright_delay;
    shimmer->dimmed_delay = dimmed_delay;

    shimmer->fade_step_num = LEDS_BR_NUM;
    shimmer->fade_step_delay = fade_delay / shimmer->fade_step_num;

    shimmer->fade_steps_left = shimmer->fade_step_num;
    shimmer->fade_step_delay_left = shimmer->fade_step_delay;

    shimmer->duration = duration;
    shimmer->led_list = led_list;
    shimmer->led_num = led_num;
    shimmer->delay = 0;
}

bool
anim_fx_shimmer_step(struct anim_fx_shimmer *shimmer, unsigned int *pdelay)
{
    uint8_t i;
    struct anim_fx_shimmer_led *led;
    struct anim_fx_shimmer_stage *stage_list;
    unsigned int delay;

    /* If fading in/out */
    if (shimmer->fade_steps_left > 0) {
        /* Decrease time to next fade step */
        shimmer->fade_step_delay_left -= shimmer->delay;
        /* If this step is over */
        if (shimmer->fade_step_delay_left == 0) {
            /* Decrease steps */
            shimmer->fade_steps_left--;
            /* Re-arm step timer */
            shimmer->fade_step_delay_left = shimmer->fade_step_delay;
        }
    /* Else, if we're in a limited-time effect body */
    } else if (shimmer->duration > 0 && shimmer->duration < UINT_MAX) {
        /* Decrease duration of effect body */
        shimmer->duration -= shimmer->delay;
        /* If effect body is over */
        if (shimmer->duration == 0) {
            /* Start fade-out */
            shimmer->fade_steps_left = shimmer->fade_step_num;
        }
    }

    /*
     * Advance the state of every dimmed LED
     * and determine delay to next update
     */
    delay = UINT_MAX;
    for (i = 0; i < shimmer->led_num; i++) {
        led = &shimmer->led_list[i];
        stage_list = led->stage_list;

        /* Subtract elapsed delay */
        led->delay_left -= shimmer->delay;

        /* While the current step has no delay left */
        while (led->delay_left == 0) {
            /* While the current stage has no steps left */
            while (led->steps_left == 0) {
                /* If cycle is over */
                if (led->stage_idx >= (ANIM_FX_SHIMMER_STAGE_IDX_NUM - 1)) {
                    /* Determine bright state delay */
                    stage_list[ANIM_FX_SHIMMER_STAGE_IDX_BRIGHT].step_delay =
                        ((prng_next() & 0xffff) * shimmer->bright_delay) >> 16;
                    /* Determine dim state delay */
                    stage_list[ANIM_FX_SHIMMER_STAGE_IDX_DIMMED].step_delay =
                        ((prng_next() & 0xffff) * shimmer->dimmed_delay) >> 16;
                    /* Restart */
                    led->stage_idx = 0;
                } else {
                    /* Move onto next stage */
                    led->stage_idx++;
                }
                led->steps_left = stage_list[led->stage_idx].step_num;
            }
            led->steps_left--;
            led->delay_left = stage_list[led->stage_idx].step_delay;
            led->next_br = led->br + stage_list[led->stage_idx].step_br_off;
        }

        /* Determine delay to next update */
        if (led->delay_left < delay) {
            delay = led->delay_left;
        }
    }

    /* If fading-in/out */
    if (shimmer->fade_steps_left > 0) {
        /* Schedule for fading step boundary */
        delay = MIN(delay, shimmer->fade_step_delay_left);
    /* Else, if we're in a limited-time effect body */
    } else if (shimmer->duration > 0 && shimmer->duration < UINT_MAX) {
        /* Schedule for effect body boundary */
        delay = MIN(delay, shimmer->duration);
    }

    /*
     * Schedule LED updates
     */
    for (i = 0; i < shimmer->led_num; i++) {
        led = &shimmer->led_list[i];
        /* If the LED is changing on the next step */
        if (led->delay_left == delay) {
            led->br = led->next_br;
        }
        /* If fading-in/out */
        if (shimmer->fade_steps_left > 0) {
            /* If fading in */
            if (shimmer->duration > 0) {
                LEDS_BR[led->idx] = (unsigned int)led->br *
                                    (shimmer->fade_step_num -
                                     shimmer->fade_steps_left + 1) /
                                    shimmer->fade_step_num;
            } else {
                LEDS_BR[led->idx] = (unsigned int)led->br *
                                    (shimmer->fade_steps_left - 1) /
                                    shimmer->fade_step_num;
            }
        } else {
            LEDS_BR[led->idx] = (unsigned int)led->br;
        }
    }

    *pdelay = (shimmer->delay = delay);
    return (shimmer->duration == 0 &&
            shimmer->fade_steps_left == 1 &&
            shimmer->fade_step_delay_left == delay);
}
