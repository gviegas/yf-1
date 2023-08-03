/*
 * YF
 * yf-kfanim.h
 *
 * Copyright © 2023 Gustavo C. Viegas.
 */

#ifndef YF_YF_KFANIM_H
#define YF_YF_KFANIM_H

#include "yf/com/yf-defs.h"

#include "yf-node.h"
#include "yf-vector.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a keyframe animation.
 */
typedef struct yf_kfanim yf_kfanim_t;

/**
 * Interpolation methods.
 */
#define YF_KFERP_STEP   0
#define YF_KFERP_LINEAR 1

/**
 * Properties that can be animated.
 */
#define YF_KFPROP_T 0
#define YF_KFPROP_R 1
#define YF_KFPROP_S 2

/**
 * Type defining input values for an animation.
 */
typedef struct yf_kfin {
    float *timeline;
    unsigned n;
} yf_kfin_t;

/**
 * Type defining output values for an animation.
 */
typedef struct yf_kfout {
    int kfprop;
    union {
        yf_vec3_t *t;
        yf_vec4_t *r;
        yf_vec3_t *s;
    };
    unsigned n;
} yf_kfout_t;

/**
 * Type defining an animation act.
 */
typedef struct yf_kfact {
    int kferp;
    /* Indices in the 'yf_kfin_t' and 'yf_kfout_t' arrays used to
       initialize an animation. The number of samples must match. */
    unsigned in_i;
    unsigned out_i;
} yf_kfact_t;

/**
 * Initializes a new animation.
 *
 * @param inputs: The 'yf_kfin_t' array containing all keyframe inputs
 *  needed by 'acts'.
 * @param input_n: The length of 'inputs'. Must be at least one.
 * @param outputs: The 'yf_kfout_t' array containing all keyframe outputs
 *  needed by 'acts'.
 * @param output_n: The length of 'outputs'. Must be at least one.
 * @param acts: The 'yf_kfact_t' array referencing 'inputs' and 'outputs'
 *  that define the keyframe animation.
 * @param act_n: The length of 'acts'. Must be at least one.
 * @return: On success, returns a new animation. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_kfanim_t *yf_kfanim_init(const yf_kfin_t *inputs, unsigned input_n,
                            const yf_kfout_t *outputs, unsigned output_n,
                            const yf_kfact_t *acts, unsigned act_n);

/**
 * Gets the target of an animation's act.
 *
 * @param anim: The animation.
 * @param act: The index in the 'yf_kfact_t' array of 'anim' indicating
 *  the act whose target is to be retrieved.
 * @return: If 'act' is an invalid index, returns 'NULL' and sets the global
 *  error to 'YF_ERR_INVARG'. Otherwise, returns the node currently set as
 *  target for 'act', or 'NULL' if none is set.
 */
yf_node_t *yf_kfanim_gettarget(yf_kfanim_t *anim, unsigned act);

/**
 * Sets the target of an animation's act.
 *
 * @param anim: The animation.
 * @param act: The index in the 'yf_kfact_t' array of 'anim' indicating
 *  the act whose target is to be set.
 * @param target: The node to set as target. Can be 'NULL'.
 * @return: If 'act' is an invalid index, returns a non-zero value and
 *  sets the global error to 'YF_ERR_INVARG'. Otherwise, zero is returned.
 */
int yf_kfanim_settarget(yf_kfanim_t *anim, unsigned act, yf_node_t *target);

/**
 * Gets the inputs of an animation.
 *
 * @param anim: The animation.
 * @param n: The destination for the array size.
 * @return: The 'yf_kfin_t' array of 'anim'.
 */
const yf_kfin_t *yf_kfanim_getins(yf_kfanim_t *anim, unsigned *n);

/**
 * Gets the outputs of an animation.
 *
 * @param anim: The animation.
 * @param n: The destination for the array size.
 * @return: The 'yf_kfout_t' array of 'anim'.
 */
const yf_kfout_t *yf_kfanim_getouts(yf_kfanim_t *anim, unsigned *n);

/**
 * Gets the acts of an animation.
 *
 * @param anim: The animation.
 * @param n: The destination for the array size.
 * @return: The 'yf_kfact_t' array of 'anim'.
 */
const yf_kfact_t *yf_kfanim_getacts(yf_kfanim_t *anim, unsigned *n);

/**
 * Applies a keyframe animation.
 *
 * @param anim: The animation.
 * @param frame_tm: The keyframe time, in seconds.
 * @return: The difference between the duration of 'anim' and 'frame_tm'.
 */
float yf_kfanim_apply(yf_kfanim_t *anim, float frame_tm);

/**
 * Deinitializes an animation.
 *
 * @param anim: The animation to deinitialize. Can be 'NULL'.
 */
void yf_kfanim_deinit(yf_kfanim_t *anim);

YF_DECLS_END

#endif /* YF_YF_KFANIM_H */
