/*
 * Card animation
 */

#include "anim.h"
#include "leds.h"
#include <misc.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>

/**
 * Prototype for an effect-stepping function.
 *
 * Note that any effect-stepping function using static variables to keep
 * state makes the effect non-reentrant, and cannot be used in more than one
 * thread.
 *
 * @param first True if this is the function invocation for the first step.
 * @param pnext Location of the pointer to this function, and for the pointer
 *              to the next effect-stepping function to call, after the
 *              returned delay elapsed.
 *
 * @return The delay after which the function pointed to by pnext will be
 *         called.
 */
typedef unsigned int (*anim_fx_fn)(bool first, void **pnext);

/**
 * Stop animation forever.
 */
static unsigned int
anim_fx_stop(bool first, void **pnext_fx)
{
    (void)first;
    (void)pnext_fx;
    return 3600000;
}

/**
 * Blink stars forever.
 */
static unsigned int
anim_fx_stars_blink(bool first, void **pnext_fx)
{
    (void)first;
    *pnext_fx = anim_fx_stop;
    return 0;
}

/**
 * Fade in the stars to 3/4 of max brightness over three seconds,
 * then start blinking.
 */
static unsigned int
anim_fx_stars_fade_in(bool first, void **pnext_fx)
{
    static unsigned int step;
    size_t i;

    if (first) {
        step = 0;
    }

    for (i = 0; i < ARRAY_SIZE(LEDS_STARS_LIST); i++) {
        LEDS_BR[LEDS_STARS_LIST[i]] = step;
    }

    step++;

    if (step >= (LEDS_BR_NUM * 3 / 4)) {
        *pnext_fx = anim_fx_stars_blink;
    }

    return 3000 * 4 / (LEDS_BR_NUM * 3);
}

static unsigned int
anim_fx_balls_wave(bool first, void **pnext_fx)
{
    static ssize_t step;
    ssize_t i, j, k;
    uint8_t br;
    unsigned int delay;

    (void)pnext_fx;

    if (first) {
        step = -2;
    }

    /* For each line of balls */
    for (i = 0; i < (ssize_t)ARRAY_SIZE(LEDS_BALLS_SWNE_LINE_LIST); i++) {
        switch (i - step) {
            case 0:
                br = LEDS_BR_MAX;
                break;
            case -1:
            case 1:
                br = LEDS_BR_MAX * 5 / 6;
                break;
            default:
                br = LEDS_BR_MAX * 3 / 4;
                break;
        }
        for (j = 0;
             (k = LEDS_BALLS_SWNE_LINE_LIST[i][j]) != LEDS_IDX_INVALID;
             j++) {
            LEDS_BR[k] = br;
        }
    }

    delay = (step == -2)
                ? 2000
                : (1000 / (ARRAY_SIZE(LEDS_BALLS_SWNE_LINE_LIST) + 4));

    step++;
    if (step >= (ssize_t)ARRAY_SIZE(LEDS_BALLS_SWNE_LINE_LIST) + 2) {
        step = -2;
    }

    return delay;
}

/** Fade in the topper to max brightness */
static unsigned int
anim_fx_topper_fade_in(bool first, void **pnext_fx)
{
    static uint8_t step;

    if (first) {
        step = 0;
    }

    LEDS_BR[LEDS_TOPPER] = step;
    step++;

    if (step == LEDS_BR_MAX) {
        *pnext_fx = anim_fx_stop;
    }

    return 1000 / LEDS_BR_NUM;
}

/** Fade in the balls to 3/4 of max brightness */
static unsigned int
anim_fx_balls_fade_in(bool first, void **pnext_fx)
{
    static unsigned int step;
    size_t i, j, k;
    uint8_t br;

    if (first) {
        step = 0;
    }

    for (i = 0; i < ARRAY_SIZE(LEDS_BALLS_SWNE_LINE_LIST); i++) {
        br = (step > i)
                    ? ((step < i + 4)
                            ? ((step - i) * (LEDS_BR_MAX * 3 )) >> 4
                            : (LEDS_BR_MAX * 3 / 4))
                    : 0;
        for (j = 0;
             (k = LEDS_BALLS_SWNE_LINE_LIST[i][j]) != LEDS_IDX_INVALID;
             j++) {
            LEDS_BR[k] = br;
        }
    }

    step++;

    if (step >= ARRAY_SIZE(LEDS_BALLS_SWNE_LINE_LIST) + 4) {
        *pnext_fx = anim_fx_balls_wave;
    }

    return 1500 / (ARRAY_SIZE(LEDS_BALLS_SWNE_LINE_LIST) + 4);
}

/** Animation thread state */
struct anim_thread {
    /** Current effect-stepping function */
    anim_fx_fn      fx;
    /**
     * True if the effect-stepping function wasn't called on the previous
     * step in this thread.
     */
    bool            first;
    /**
     * Delay in milliseconds until the next step (invocation of the stepping
     * function) in this thread.
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
        .fx = anim_fx_stars_fade_in,
        .first = true,
        .delay = 0,
    };
    ANIM_THREADS[1] = (struct anim_thread){
        .fx = anim_fx_topper_fade_in,
        .first = true,
        .delay = 1000,
    };
    ANIM_THREADS[2] = (struct anim_thread){
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
        for (i = 0; i < ARRAY_SIZE(ANIM_THREADS); i++) {
            thread = &ANIM_THREADS[i];
            delay = thread->delay;
            if (delay == 0) {
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

    return ANIM_DELAY_MIN;
}
