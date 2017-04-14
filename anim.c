/*
 * Card animation
 */

#include "anim.h"
#include "leds.h"
#include <prng.h>
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
    /* Possible state of a dimmed star */
    enum state {
        /* Bright, waiting to be dimmed */
        STATE_WAIT,
        /* Dimmed, waiting to be restored */
        STATE_DIM,
        /* Restored, resting */
        STATE_REST,
        /* Number of states, not a valid state */
        STATE_NUM
    };

    /* State of a dimmed star */
    struct star {
        /* Star LED index */
        uint8_t         led;
        /* Current state */
        enum state      state;
        /* Delay for each state */
        unsigned int    state_delay[STATE_NUM];
    };

    /* State of the stars currently being dimmed */
    static struct star star_list[4];

    /* Delay since the last invocation */
    static unsigned int delay;

    size_t i, j;
    struct star *star;
    unsigned int new_delay;

    (void)pnext_fx;

    if (first) {
        delay = 0;
        for (i = 0; i < ARRAY_SIZE(star_list); i++) {
            star = &star_list[i];
            star->state = STATE_REST;
            for (j = 0; j < ARRAY_SIZE(star->state_delay); j++) {
                star->state_delay[j] = 0;
            }
        }
    }

    /*
     * Advance the state of every dimmed star
     */
    new_delay = UINT_MAX;
    for (i = 0; i < ARRAY_SIZE(star_list); i++) {
        star = &star_list[i];
        /* If the current state delay is over */
        if ((star->state_delay[star->state] -= delay) == 0) {
            /* Switch on the state that finished */
            switch (star->state) {
                /* Waiting is over */
                case STATE_WAIT:
                    /* Dimmed now */
                    star->state = STATE_DIM;
                    break;
                /* Dimming is over */
                case STATE_DIM:
                    /* Resting now */
                    star->state = STATE_REST;
                    break;
                /* Resting is over */
                case STATE_REST:
                    /* Pick an LED */
                    star->led = LEDS_STARS_LIST[
                                    (prng_next() >> 8) *
                                    (LEDS_STARS_NUM - 1) /
                                    (PRNG_MAX >> 8)];
                    /* Pick the wait time */
                    star->state_delay[STATE_WAIT] = (prng_next() & 0xf) << 9;
                    /* Set the dim time */
                    star->state_delay[STATE_DIM] = 0x200;
                    /* Calculate the rest time */
                    star->state_delay[STATE_REST] =
                        0x2000 - star->state_delay[STATE_WAIT];
                    /* Waiting now */
                    star->state = STATE_WAIT;
                    break;
                default:
                    break;
            }
        }
        if (star->state_delay[star->state] < new_delay) {
            new_delay = star->state_delay[star->state];
        }
    }

    /*
     * Schedule LED updates of all the stars changing next
     */
    for (i = 0; i < ARRAY_SIZE(star_list); i++) {
        star = &star_list[i];
        /* If the star is changing on the next step */
        if (star->state_delay[star->state] == new_delay) {
            /* Switch on the state that is going to be finished */
            switch (star->state) {
                /* Waiting will be finished */
                case STATE_WAIT:
                    /* Schedule dimming */
                    LEDS_BR[star->led] = LEDS_BR_MAX / 2;
                    break;
                /* Dimming will be finished */
                case STATE_DIM:
                    /* Schedule resting */
                    LEDS_BR[star->led] = LEDS_BR_MAX * 3 / 4;
                    break;
                /* Resting will be finished */
                case STATE_REST:
                    /* No need to change the LED */
                    break;
                default:
                    break;
            }
        }
    }

    return (delay = new_delay);
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
    size_t i;

    if (first) {
        step = 0;
    }

    for (i = 0; i < ARRAY_SIZE(LEDS_TOPPER_LIST); i++) {
        LEDS_BR[LEDS_TOPPER_LIST[i]] = step;
    }
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
