/*
 * Card animation effect-stepping functions
 */

#ifndef _ANIM_FX_H
#define _ANIM_FX_H

#include <stdbool.h>

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

/** Stop animation forever */
extern unsigned int anim_fx_stop(bool first, void **pnext_fx);

/** Blink stars forever */
extern unsigned int anim_fx_stars_blink(bool first, void **pnext_fx);

/** Fade in the stars to 3/4 brightness, then blink forever */
extern unsigned int anim_fx_stars_fade_in(bool first, void **pnext_fx);

/** Periodically send a wave through the balls, forever */
extern unsigned int anim_fx_balls_wave(bool first, void **pnext_fx);

/** Fade in the topper to max brightness, then stop */
extern unsigned int anim_fx_topper_fade_in(bool first, void **pnext_fx);

/** Fade in the balls to 3/4 of max brightness, then send waves */
extern unsigned int anim_fx_balls_fade_in(bool first, void **pnext_fx);

#endif /* _ANIM_FX_H */