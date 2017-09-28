/*
 * Card's LED scripted animation
 */

#include "anim_fx_script.h"
#include "leds.h"
#include <prng.h>
#include <misc.h>
#include <limits.h>

/**
 * Initialize a per-LED segment state.
 *
 * @param seg       The segment description to initialize for.
 * @param led_seg   The per-LED segment state to initialize.
 */
void
anim_fx_script_led_seg_init(const struct anim_fx_script_seg *seg,
                            struct anim_fx_script_led_seg *led_seg)
{
#define GEN_FIELD(_field) \
    do {                                                                \
        unsigned int min;                                               \
        unsigned int max;                                               \
        unsigned int num;                                               \
                                                                        \
        if (seg->_field##_min <= seg->_field##_max) {                   \
            min = seg->_field##_min;                                    \
            max = seg->_field##_max;                                    \
        } else {                                                        \
            max = seg->_field##_min;                                    \
            min = seg->_field##_max;                                    \
        }                                                               \
        led_seg->_field = min;                                          \
        num = max - min;                                                \
        if (num > 0) {                                                  \
            led_seg->_field += (((prng_next() & 0xffff) * num) >> 16);  \
        }                                                               \
    } while (0)

    GEN_FIELD(step_num);
    GEN_FIELD(step_delay);
}

/**
 * Initialize a per-LED segment state list.
 *
 * @param seg_list      The segment description list to initialize for.
 * @param led_seg_list  The per-LED segment state list to initialize.
 * @param seg_num       Number of segments.
 */
void
anim_fx_script_led_seg_list_init(const struct anim_fx_script_seg *seg_list,
                                 struct anim_fx_script_led_seg *led_seg_list,
                                 uint8_t seg_num)
{
    uint8_t i;

    for (i = 0; i < seg_num; i++) {
        anim_fx_script_led_seg_init(&seg_list[i], &led_seg_list[i]);
    }
}

void
anim_fx_script_init(struct anim_fx_script *script,
                    uint8_t seg_num,
                    const struct anim_fx_script_seg *seg_list,
                    uint8_t led_num,
                    const uint8_t *idx_list,
                    struct anim_fx_script_led *led_list,
                    struct anim_fx_script_led_seg *led_seg_list_list,
                    uint8_t br,
                    unsigned int fade_delay,
                    unsigned int duration)
{
    size_t i;

    /* Initialize LED list */
    for (i = 0; i < led_num; i++) {
        struct anim_fx_script_led *led = &led_list[i];

        /* Assign the LED index */
        led->idx = idx_list[i];
        /* Assign segment state list */
        led->seg_list = led_seg_list_list + (i * seg_num);

        /* Position at the end of the cycle */
        led->seg_idx = script->seg_num - 1;
        led->steps_left = 0;
        led->delay_left = 0;

        /* Initialize brightness */
        led->br = br;
    }

    /* Initialize script state */
    script->seg_num = seg_num;
    script->seg_list = seg_list;

    script->led_num = led_num;
    script->led_list = led_list;

    script->fade_step_num = LEDS_BR_NUM;
    script->fade_step_delay = fade_delay / script->fade_step_num;

    script->fade_steps_left = script->fade_step_num;
    script->fade_step_delay_left = script->fade_step_delay;

    script->duration = duration;
    script->delay = 0;
}

bool
anim_fx_script_step(struct anim_fx_script *script, unsigned int *pdelay)
{
    uint8_t i;
    struct anim_fx_script_led *led;
    unsigned int delay;

    /* If fading in/out */
    if (script->fade_steps_left > 0) {
        /* Decrease time to next fade step */
        script->fade_step_delay_left -= script->delay;
        /* If this step is over */
        if (script->fade_step_delay_left == 0) {
            /* Decrease steps */
            script->fade_steps_left--;
            /* Re-arm step timer */
            script->fade_step_delay_left = script->fade_step_delay;
        }
    /* Else, if we're in a limited-time effect body */
    } else if (script->duration > 0 && script->duration < UINT_MAX) {
        /* Decrease duration of effect body */
        script->duration -= script->delay;
        /* If effect body is over */
        if (script->duration == 0) {
            /* Start fade-out */
            script->fade_steps_left = script->fade_step_num;
        }
    }

    /*
     * Advance the state of every animated LED
     * and determine delay to next update
     */
    delay = UINT_MAX;
    for (i = 0; i < script->led_num; i++) {
        led = &script->led_list[i];

        /* Subtract elapsed delay */
        led->delay_left -= script->delay;

        /* While the current step has no delay left */
        while (led->delay_left == 0) {
            /* While the current seg has no steps left */
            while (led->steps_left == 0) {
                /* If cycle is over */
                if (led->seg_idx >= script->seg_num - 1) {
                    /* Reinitialize segment states */
                    anim_fx_script_led_seg_list_init(script->seg_list,
                                                     led->seg_list,
                                                     script->seg_num);
                    /* Restart */
                    led->seg_idx = 0;
                } else {
                    /* Move onto next seg */
                    led->seg_idx++;
                }
                led->steps_left = led->seg_list[led->seg_idx].step_num;
            }
            led->steps_left--;
            led->delay_left = led->seg_list[led->seg_idx].step_delay;
            led->next_br = led->br + script->seg_list[led->seg_idx].step_br_off;
        }

        /* Determine delay to next update */
        if (led->delay_left < delay) {
            delay = led->delay_left;
        }
    }

    /* If fading-in/out */
    if (script->fade_steps_left > 0) {
        /* Schedule for fading step boundary */
        delay = MIN(delay, script->fade_step_delay_left);
    /* Else, if we're in a limited-time effect body */
    } else if (script->duration > 0 && script->duration < UINT_MAX) {
        /* Schedule for effect body boundary */
        delay = MIN(delay, script->duration);
    }

    /*
     * Schedule LED updates
     */
    for (i = 0; i < script->led_num; i++) {
        led = &script->led_list[i];
        /* If the LED is changing on the next step */
        if (led->delay_left == delay) {
            led->br = led->next_br;
        }
        /* If fading-in/out */
        if (script->fade_steps_left > 0) {
            /* If fading in */
            if (script->duration > 0) {
                LEDS_BR[led->idx] = (unsigned int)led->br *
                                    (script->fade_step_num -
                                     script->fade_steps_left + 1) /
                                    script->fade_step_num;
            } else {
                LEDS_BR[led->idx] = (unsigned int)led->br *
                                    (script->fade_steps_left - 1) /
                                    script->fade_step_num;
            }
        } else {
            LEDS_BR[led->idx] = (unsigned int)led->br;
        }
    }

    *pdelay = (script->delay = delay);
    return (script->duration == 0 &&
            script->fade_steps_left == 1 &&
            script->fade_step_delay_left == delay);
}
