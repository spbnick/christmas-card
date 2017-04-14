/*
 * Card animation
 */

#include "anim.h"
#include "anim_fx.h"
#include "leds.h"
#include <misc.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>

/** Animation thread state */
struct anim_thread {
    /** Array of indexes of LEDs that this thread modifies */
    const uint8_t  *led_list;
    /** Number of indexes of LEDs in led_list */
    uint8_t         led_num;
    /** Current effect-stepping function */
    anim_fx_fn      fx;
    /**
     * True if the effect-stepping function wasn't called on the previous
     * step in this thread.
     */
    bool            first;
    /**
     * Delay in milliseconds until the brightness of LEDs that this thread
     * modifies becomes active, and the effect-stepping function is called.
     */
    unsigned int    delay;
};

/** List of thread states */
struct anim_thread  ANIM_THREADS[3];
/** Minimum delay until the next animation step across all threads */
unsigned int ANIM_DELAY_MIN;

void
anim_init(void)
{
    ANIM_THREADS[0] = (struct anim_thread){
        .led_list = LEDS_STARS_LIST,
        .led_num = LEDS_STARS_NUM,
        .fx = anim_fx_stars_fade_in,
        .first = true,
        .delay = 0,
    };
    ANIM_THREADS[1] = (struct anim_thread){
        .led_list = LEDS_TOPPER_LIST,
        .led_num = LEDS_TOPPER_NUM,
        .fx = anim_fx_topper_fade_in,
        .first = true,
        .delay = 1000,
    };
    ANIM_THREADS[2] = (struct anim_thread){
        .led_list = LEDS_BALLS_LIST,
        .led_num = LEDS_BALLS_NUM,
        .fx = anim_fx_balls_fade_in,
        .first = true,
        .delay = 1500,
    };
    ANIM_DELAY_MIN = 0;
}

unsigned int
anim_step(void)
{
    size_t i;
    unsigned int delay;
    struct anim_thread *thread;
    anim_fx_fn fx;

    /* Subtract time passed since the last step */
    for (i = 0; i < ARRAY_SIZE(ANIM_THREADS); i++) {
        ANIM_THREADS[i].delay -= ANIM_DELAY_MIN;
    }

    /* While we have threads to run immediately */
    do {
        ANIM_DELAY_MIN = UINT_MAX;
        /* Advance each thread */
        for (i = 0; i < ARRAY_SIZE(ANIM_THREADS); i++) {
            thread = &ANIM_THREADS[i];
            delay = thread->delay;
            /* If the previous thread step is over */
            if (delay == 0) {
                /* Render the previous state into the swapped buffer */
                leds_render_list(thread->led_list, thread->led_num);
                /* Calculate next step */
                fx = thread->fx;
                delay = fx(thread->first, (void **)&fx);
                thread->delay = delay;
                thread->first = thread->fx != fx;
                thread->fx = fx;
            }
            if (delay < ANIM_DELAY_MIN) {
                ANIM_DELAY_MIN = delay;
            }
        }
    } while (ANIM_DELAY_MIN == 0);

    /* Render threads to come into effect next animation step */
    for (i = 0; i < ARRAY_SIZE(ANIM_THREADS); i++) {
        thread = &ANIM_THREADS[i];
        if (thread->delay == ANIM_DELAY_MIN) {
            leds_render_list(thread->led_list, thread->led_num);
        }
    }

    return ANIM_DELAY_MIN;
}
