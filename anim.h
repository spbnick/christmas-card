/*
 * Card animation
 */

#ifndef _ANIM_H
#define _ANIM_H


/**
 * Initialize and begin animation.
 */
extern void anim_init(void);

/**
 * Render the next animation step into the inactive LEDs bank.
 *
 * @return Time in milliseconds the rendered animation step
 *         should begin output since the previous step was.
 */
extern unsigned int anim_step(void);

#endif /* _ANIM_H */
