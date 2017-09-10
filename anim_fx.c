/*
 * Card animation effect-stepping functions
 */

#include "anim_fx_shimmer.h"
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
        STATE_DIMMED,
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
        [STATE_DIMMED]      = LEDS_BR_MAX * 5 / 8,
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
                star->state_delay[STATE_DIMMED] = 0x100;
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
anim_fx_balls_fade_in_and_out(bool first, void **pnext_fx)
{
    static enum {
        FADE_IN,
        WAIT,
        FADE_OUT
    } stage;
    static int step;
    static const uint8_t br_steps[] = {
        0,
        LEDS_BR_MAX / 4,
        LEDS_BR_MAX * 2 / 4,
        LEDS_BR_MAX * 3 / 4,
        LEDS_BR_MAX
    };
    int idx;
    int br_idx;
    int i;
    int j;

    if (first) {
        stage = FADE_IN;
        step = 0;
    }

    if (stage == WAIT) {
        stage = FADE_OUT;
        return 10000;
    }

    idx = 0;
    for (i = (int)ARRAY_SIZE(LEDS_BALLS_SWNE_LINE_LIST) - 1; i >= 0; i--) {
        for (j = 0;
             j < (int)ARRAY_SIZE(LEDS_BALLS_SWNE_LINE_LIST[i]) &&
             LEDS_BALLS_SWNE_LINE_LIST[i][j] != LEDS_IDX_INVALID;
             j++) {
            br_idx = step - idx;
            if (br_idx < 0) {
                br_idx = 0;
            } else if (br_idx >= (int)ARRAY_SIZE(br_steps)) {
                br_idx = ARRAY_SIZE(br_steps) - 1;
            }
            LEDS_BR[LEDS_BALLS_SWNE_LINE_LIST[i][j]] = br_steps[br_idx];
            idx++;
        }
    }

    if (stage == FADE_IN) {
        step++;
        if (step == LEDS_BALLS_NUM + ARRAY_SIZE(br_steps) - 1) {
            stage = WAIT;
        }
    } else if (stage == FADE_OUT) {
        step--;
        if (step == 0) {
            *pnext_fx = anim_fx_balls_random;
        }
    }

    return 1500 / (LEDS_BALLS_NUM + ARRAY_SIZE(br_steps) - 1);
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
    static enum leds_balls_color prev_color;
    static enum leds_balls_color cur_color;
    static unsigned int hue;
    enum leds_balls_color color;
    uint8_t br;
    size_t i;

    (void)pnext_fx;

    if (first) {
        step = 0;
        prev_color = LEDS_BALLS_COLOR_NUM;
        cur_color = 0;
        hue = 0;
    }

    if (step == 12) {
        cur_color = LEDS_BALLS_COLOR_NUM;
    }

    for (color = 0; color < ARRAY_SIZE(LEDS_BALLS_COLOR_LIST); color++) {
        if (color == cur_color) {
            br = ((hue + 1) * LEDS_BR_MAX) >> 3 ;
        } else if (color == prev_color) {
            br = ((7 - hue) * LEDS_BR_MAX) >> 3;
        } else {
            br = 0;
        }
        for (i = 0; i < ARRAY_SIZE(LEDS_BALLS_COLOR_LIST[color]); i++) {
            LEDS_BR[LEDS_BALLS_COLOR_LIST[color][i]] = br;
        }
    }

    hue++;
    if (hue >= 8) {
        hue = 0;
        prev_color = cur_color;
        cur_color++;
        if (cur_color >= LEDS_BALLS_COLOR_NUM) {
            cur_color = 0;
            step++;
            if (step > 12) {
                *pnext_fx = anim_fx_balls_random;
            }
        }
    }

    return hue == 1 ? 1100 : 50;
}

unsigned int
anim_fx_balls_snow(bool first, void **pnext_fx)
{
    /* True if filling, false if emptying */
    static bool fill;
    /* The current row being filled/emtpied, destination/origin row */
    static uint8_t row;
    /* Current row column status */
    static bool idle_cols[LEDS_BALLS_COL_NUM];
    /* Number of unfilled/unemptied columns at the current row */
    static uint8_t idle_cols_num;
    /* Current falling ball column */
    static uint8_t ball_col;
    /* Previous falling ball row */
    static uint8_t prev_ball_row;
    /* Current falling ball row */
    static uint8_t ball_row;
    /* Ball brightness */
    static uint8_t ball_br;

    size_t i;

    if (first) {
        /* We're filling */
        fill = true;
        /* Below the bottom row */
        row = LEDS_BALLS_ROW_NUM;
        /* No unfilled columns remaining in the row */
        idle_cols_num = 0;
        /* Ball in the below-the-bottom row */
        ball_row = row;
        /* Ball finished lighting */
        ball_br = LEDS_BR_MAX;
    }

    /* Else, if the ball in current position is lighted fully */
    if (ball_br >= LEDS_BR_MAX) {
        /* Reset ball brightness */
        ball_br = 0;
        /* Current row becomes previous row */
        prev_ball_row = ball_row;

        /* If the ball is in flight still */
        if (fill ? (ball_row < row) : (ball_row <= LEDS_BALLS_ROW_NUM)) {
            /* Move to the next row */
            ball_row++;
        /* Else */
        } else {
            /* If there are no unfilled columns left */
            if (idle_cols_num == 0) {
                /* If we are at the top */
                if (row == 0) {
                    if (fill) {
                        /* Switch to emptying */
                        fill = false;
                        /* Emptying row below the bottom */
                        row = LEDS_BALLS_ROW_NUM + 1;
                        /* No unemptied columns left */
                        idle_cols_num = 0;
                        /* Ball is below the bottom */
                        ball_row = row;
                        /* Completely lighted */
                        ball_br = LEDS_BR_MAX;
                        /* Wait before emptying */
                        return 7000;
                    } else {
                        /* Run random effects forever */
                        *pnext_fx = anim_fx_balls_random;
                        /* Wait to allow satisfaction settle a little */
                        return 3000;
                    }
                }
                /* Move up a row */
                row--;
                /* Count and mark available columns */
                for (i = 0; i < LEDS_BALLS_COL_NUM; i++) {
                    idle_cols[i] = LEDS_BALLS_ROW_LIST[row][i] != LEDS_IDX_INVALID;
                    if (idle_cols[i]) {
                        idle_cols_num++;
                    }
                }
            }
            /*
             * At the current fill/emptying row,
             * choose a column from unfilled/unemptied
             */
            i = ((prng_next() & 0xffff) * (uint32_t)idle_cols_num) >> 16;
            for (ball_col = 0; ball_col < LEDS_BALLS_COL_NUM; ball_col++) {
                if (idle_cols[ball_col]) {
                    if (i == 0) {
                        break;
                    } else {
                        i--;
                    }
                }
            }
            idle_cols[ball_col] = false;
            idle_cols_num--;
            if (fill) {
                ball_row = 0;
            } else {
                prev_ball_row = row;
                ball_row = row + 1;
            }
        }

        /* Find the row where column is present */
        for (; (fill ? (ball_row < row) : (ball_row < LEDS_BALLS_ROW_NUM)) &&
               LEDS_BALLS_ROW_LIST[ball_row][ball_col] == LEDS_IDX_INVALID;
             ball_row++);
    }

    /* Increase ball brightness */
    ball_br += LEDS_BR_NUM / 8;
    if (ball_br > LEDS_BR_MAX) {
        ball_br = LEDS_BR_MAX;
    }

    /* Update brightness of the previous ball, if any */
    if (fill ? (prev_ball_row < ball_row)
             : (prev_ball_row < LEDS_BALLS_ROW_NUM)) {
        LEDS_BR[LEDS_BALLS_ROW_LIST[prev_ball_row][ball_col]] =
            LEDS_BR_MAX - ball_br;
    }

    /* Update brightness of the current ball, if any */
    if (ball_row < LEDS_BALLS_ROW_NUM) {
        LEDS_BR[LEDS_BALLS_ROW_LIST[ball_row][ball_col]] = ball_br;
    }

    return 75;
}

unsigned int
anim_fx_balls_shimmer(bool first, void **pnext_fx)
{
    static struct anim_fx_shimmer shimmer;
    static struct anim_fx_shimmer_led led_list[ARRAY_SIZE(LEDS_BALLS_LIST)];
    unsigned int delay;

    if (first) {
        anim_fx_shimmer_init(&shimmer,
                             led_list, LEDS_BALLS_LIST, ARRAY_SIZE(led_list),
                             LEDS_BR_MAX, 10000, 300, 7000, 30000);
    }

    if (anim_fx_shimmer_step(&shimmer, &delay)) {
        *pnext_fx = anim_fx_balls_random;
    }
    return delay;
}

/** Pool of the balls effect-stepping functions to choose from randomly */
static const anim_fx_fn ANIM_FX_BALLS_RANDOM_POOL[] = {
    anim_fx_balls_wave,
    anim_fx_balls_glitter,
    anim_fx_balls_cycle_colors,
    anim_fx_balls_snow,
    anim_fx_balls_shimmer,
};

/** Index of the balls effect-stepping function chosen last */
static size_t ANIM_FX_BALLS_RANDOM_LAST =
                    ARRAY_SIZE(ANIM_FX_BALLS_RANDOM_POOL);

unsigned int
anim_fx_balls_random(bool first, void **pnext_fx)
{
    size_t i;

    (void)first;
    (void)pnext_fx;

    i = prng_next() % ARRAY_SIZE(ANIM_FX_BALLS_RANDOM_POOL);
    if (i == ANIM_FX_BALLS_RANDOM_LAST) {
        i = (i + 1) % ARRAY_SIZE(ANIM_FX_BALLS_RANDOM_POOL);
    }

    *pnext_fx = ANIM_FX_BALLS_RANDOM_POOL[i];
    ANIM_FX_BALLS_RANDOM_LAST = i;

    return 0;
}
