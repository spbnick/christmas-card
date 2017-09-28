/*
 * Card's LED shimmering animation script setup
 */

#include "anim_fx_shimmer.h"
#include "leds.h"
#include <prng.h>
#include <misc.h>
#include <limits.h>

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
                            unsigned int duration)
{
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

    /* Fill in stage constants */
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_BRIGHT].step_num_min = 1;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_BRIGHT].step_num_max = 1;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_BRIGHT].step_br_off = 0;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_BRIGHT].step_delay_min = 0;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_BRIGHT].step_delay_max = bright_delay;

    seg_list[ANIM_FX_SHIMMER_SEG_IDX_DIMMING].step_num_min = dim_step_num;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_DIMMING].step_num_max = dim_step_num;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_DIMMING].step_br_off = -dim_step_br_off;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_DIMMING].step_delay_min = dim_step_delay;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_DIMMING].step_delay_max = dim_step_delay;

    seg_list[ANIM_FX_SHIMMER_SEG_IDX_DIMMED].step_num_min = 1;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_DIMMED].step_num_max = 1;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_DIMMED].step_br_off = 0;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_DIMMED].step_delay_min = 0;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_DIMMED].step_delay_max = dimmed_delay;

    seg_list[ANIM_FX_SHIMMER_SEG_IDX_RESTORING].step_num_min = dim_step_num;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_RESTORING].step_num_max = dim_step_num;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_RESTORING].step_br_off = dim_step_br_off;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_RESTORING].step_delay_min =
                                                        dim_step_delay;
    seg_list[ANIM_FX_SHIMMER_SEG_IDX_RESTORING].step_delay_max =
                                                        dim_step_delay;

    /* Initialize the script */
    anim_fx_script_init(script, ANIM_FX_SHIMMER_SEG_IDX_NUM, seg_list,
                        led_num, idx_list, led_list, led_seg_list_list,
                        bright_br, fade_delay, duration);
}
