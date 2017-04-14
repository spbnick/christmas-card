/*
 * Card animation effect-stepping functions
 */

#include "anim_fx.h"
#include "leds.h"
#include <prng.h>
#include <misc.h>
#include <unistd.h>
#include <limits.h>

unsigned int
anim_fx_stop(bool first, void **pnext_fx)
{
    (void)first;
    (void)pnext_fx;
    return 3600000;
}

unsigned int
anim_fx_stars_blink(bool first, void **pnext_fx)
{
    /* Possible state of a dimmed star */
    enum state {
        /* Waiting to be dimmed */
        STATE_WAIT,
        /* Dimming, half-dimmed */
        STATE_DIMMER,
        /* Fully dimmed */
        STATE_DIM,
        /* Restoring, half-brightened */
        STATE_RESTORING,
        /* Bright (restored), resting */
        STATE_RESTORED,
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

    /* LED brightness after the end of each state */
    static const uint8_t state_next_br[STATE_NUM] = {
        [STATE_WAIT]        = LEDS_BR_MAX * 5 / 8,
        [STATE_DIMMER]      = LEDS_BR_MAX / 2,
        [STATE_DIM]         = LEDS_BR_MAX * 5 / 8,
        [STATE_RESTORING]   = LEDS_BR_MAX * 3 / 4,
        [STATE_RESTORED]    = LEDS_BR_MAX * 3 / 4,
    };

    /* State of the stars currently being dimmed */
    static struct star star_list[8];

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
            star->state = STATE_RESTORED;
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
            /* If cycle is over */
            if (star->state == STATE_RESTORED) {
                /* Pick an LED */
                star->led = LEDS_STARS_LIST[
                                (prng_next() >> 8) *
                                (LEDS_STARS_NUM - 1) /
                                (PRNG_MAX >> 8)];
                star->state_delay[STATE_WAIT] = (prng_next() & 0xf) << 9;
                star->state_delay[STATE_DIMMER] = 0x80;
                star->state_delay[STATE_DIM] = 0x100;
                star->state_delay[STATE_RESTORING] = 0x80;
                star->state_delay[STATE_RESTORED] =
                    0x2000 - star->state_delay[STATE_WAIT];
                /* Waiting now */
                star->state = STATE_WAIT;
            } else {
                star->state++;
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
            /* Schedule brightness change */
            LEDS_BR[star->led] = state_next_br[star->state];
        }
    }

    /* Call us for the next change */
    return (delay = new_delay);
}

unsigned int
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

unsigned int
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

unsigned int
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

unsigned int
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
