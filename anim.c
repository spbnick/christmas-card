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
static struct anim_thread  ANIM_THREADS[] = {
    {
        .led_list = LEDS_STARS_LIST,
        .led_num = LEDS_STARS_NUM,
        .fx = anim_fx_stars_fade_in,
        .first = true,
        .delay = 0,
    },
    {
        .led_list = LEDS_TOPPER_LIST,
        .led_num = LEDS_TOPPER_NUM,
        .fx = anim_fx_topper_fade_in,
        .first = true,
        .delay = 3000,
    },
    {
        .led_list = LEDS_BALLS_LIST,
        .led_num = LEDS_BALLS_NUM,
        .fx = anim_fx_balls_fade_in_and_out,
        .first = true,
        .delay = 1500,
    },
};

/** Delay until the next animation step across all threads */
static unsigned int ANIM_DELAY = 0;

void
anim_init(void)
{
}

unsigned int
anim_step(void)
{
    size_t i;
    unsigned int delay_next;
    struct anim_thread *thread;
    anim_fx_fn fx;

    /* Advance each thread and calculate next delay */
    delay_next = UINT_MAX;
    for (i = 0; i < ARRAY_SIZE(ANIM_THREADS); i++) {
        thread = &ANIM_THREADS[i];
        thread->delay -= ANIM_DELAY;
        /* If the previous thread step is over */
        if (thread->delay == 0) {
            /* Render the previous state into the swapped buffer */
            leds_render_list(thread->led_list, thread->led_num);
            /* Calculate next step */
            fx = thread->fx;
            thread->delay = fx(thread->first, (void **)&fx);
            thread->first = thread->fx != fx;
            thread->fx = fx;
        }
        if (thread->delay < delay_next) {
            delay_next = thread->delay;
        }
    }

    /* Render threads to come into effect next animation step */
    for (i = 0; i < ARRAY_SIZE(ANIM_THREADS); i++) {
        thread = &ANIM_THREADS[i];
        if (thread->delay == delay_next) {
            leds_render_list(thread->led_list, thread->led_num);
        }
    }

    return (ANIM_DELAY = delay_next);
}
