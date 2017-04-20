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
anim_fx_stars_shimmer(bool first, void **pnext_fx)
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
        *pnext_fx = anim_fx_stars_shimmer;
    }

    return 3000 * 4 / (LEDS_BR_NUM * 3);
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
anim_fx_balls_wave(bool first, void **pnext_fx)
{
    /*
     * Generated with
     * perl -e 'use Math::Trig;
     *          my $n=64;
     *          for (my $i=0; $i < $n; $i++) {
     *              printf("%.0f, ", sin(2*pi*$i/$n-pi/2) * 16 + 47);
     *          };
     *          print("\n")'
     */
    static const uint8_t wave[] = {
        31, 31, 31, 32, 32, 33, 34, 35, 36, 37, 38, 39, 41, 42, 44, 45,
        47, 49, 50, 52, 53, 55, 56, 57, 58, 59, 60, 61, 62, 62, 63, 63,
        63, 63, 63, 62, 62, 61, 60, 59, 58, 57, 56, 55, 53, 52, 50, 49,
        47, 45, 44, 42, 41, 39, 38, 37, 36, 35, 34, 33, 32, 32, 31, 31,
    };
    static size_t step;
    size_t i, j, k, w;
    unsigned int br;

    if (first) {
        step = 0;
    }

    /* For each line of balls */
    for (i = 0; i < ARRAY_SIZE(LEDS_BALLS_SWNE_LINE_LIST); i++) {
        w = step + (i << 2);
        /* Calculate brightness */
        br = wave[w & (ARRAY_SIZE(wave) - 1)];
        /* If fading in */
        if ((w & 0x700) == 0) {
            br = (br * ((w >> 2) & 0x3f)) >> 6;
        /* Else, if fading out */
        } else if ((w & 0x700) == 0x700) {
            br = (br * (0x40 - ((w >> 2) & 0x3f))) >> 6;
        }
        /* For each ball on the line */
        for (j = 0;
             (k = LEDS_BALLS_SWNE_LINE_LIST[i][j]) != LEDS_IDX_INVALID;
             j++) {
            LEDS_BR[k] = br;
        }
    }

    step++;

    if (step == 0x800) {
        *pnext_fx = anim_fx_balls_random;
    }

    return 50;
}

unsigned int
anim_fx_balls_glitter(bool first, void **pnext_fx)
{
    /* Possible state of a ball */
    enum state {
        /* Off */
        STATE_OFF,
        /* On */
        STATE_ON,
        /* Number of states, not a valid state */
        STATE_NUM
    };

    /* State of a bal */
    struct ball {
        /* Current state */
        enum state      state;
        /* Delay for each state */
        unsigned int    state_delay[STATE_NUM];
    };

    /* State of each ball this cycle */
    static struct ball ball_list[LEDS_BALLS_NUM];
    /* Delay since the last invocation */
    static unsigned int delay;

    static unsigned int step;

    unsigned int new_delay;
    struct ball *ball;
    size_t i, j;
    uint8_t br;

    if (first) {
        step = 0;
        delay = 0;
        for (i = 0; i < ARRAY_SIZE(ball_list); i++) {
            ball = &ball_list[i];
            ball->state = STATE_ON;
            for (j = 0; j < ARRAY_SIZE(ball->state_delay); j++) {
                ball->state_delay[j] = 0;
            }
        }
    }

    /* Calculate "on" brightness for this step */
    if ((step & 0xe00) == 0) {
        br = (step * LEDS_BR_NUM) >> 9;
    } else if ((step & 0xe00) == 0xe00) {
        br = ((0x1ff - (step & 0x1ff)) * LEDS_BR_NUM) >> 9;
    } else {
        br = LEDS_BR_MAX;
    }

    /*
     * Advance the state of every blinking ball
     */
    new_delay = UINT_MAX;
    for (i = 0; i < ARRAY_SIZE(ball_list); i++) {
        ball = &ball_list[i];
        /* If the current state delay is over */
        if ((ball->state_delay[ball->state] -= delay) == 0) {
            /* If cycle is over */
            if (ball->state == STATE_ON) {
                ball->state_delay[STATE_OFF] =
                    ((prng_next() & 0xffff) * 291) >> 16;
                ball->state_delay[STATE_ON] = 10;
                /* Off now */
                ball->state = STATE_OFF;
                /* A step is complete */
                step++;
            } else {
                ball->state++;
            }
        }
        if (ball->state_delay[ball->state] < new_delay) {
            new_delay = ball->state_delay[ball->state];
        }
    }

    /*
     * Schedule LED updates of all the balls changing next
     */
    for (i = 0; i < ARRAY_SIZE(ball_list); i++) {
        ball = &ball_list[i];
        /* If the ball is changing on the next step */
        if (ball->state_delay[ball->state] == new_delay) {
            /* Schedule brightness change */
            LEDS_BR[LEDS_BALLS_LIST[i]] =
                (ball->state == STATE_OFF) ? br : 0;
        }
    }

    if (step == 0xf00) {
        *pnext_fx = anim_fx_balls_random;
    }

    /* Call us for the next change */
    return (delay = new_delay);
}

unsigned int
anim_fx_balls_cycle_colors(bool first, void **pnext_fx)
{
    static unsigned int step;
    static enum leds_balls_color on_color;
    enum leds_balls_color color;
    size_t i;

    (void)pnext_fx;

    if (first) {
        step = 0;
        on_color = 0;
    }

    for (color = 0; color < ARRAY_SIZE(LEDS_BALLS_COLOR_LIST); color++) {
        for (i = 0; i < ARRAY_SIZE(LEDS_BALLS_COLOR_LIST[color]); i++) {
            LEDS_BR[LEDS_BALLS_COLOR_LIST[color][i]] =
                (color == on_color && step < 64) ? LEDS_BR_MAX : 0;
        }
    }

    on_color++;
    if (on_color >= ARRAY_SIZE(LEDS_BALLS_COLOR_LIST)) {
        on_color = 0;
    }

    step++;
    if (step > 64) {
        *pnext_fx = anim_fx_balls_random;
    }

    return 750;
}

unsigned int
anim_fx_balls_random(bool first, void **pnext_fx)
{
    static const anim_fx_fn pool[] = {
        anim_fx_balls_wave,
        anim_fx_balls_glitter,
        anim_fx_balls_cycle_colors,
    };

    (void)first;
    (void)pnext_fx;

    *pnext_fx = pool[prng_next() % ARRAY_SIZE(pool)];

    return 0;
}
