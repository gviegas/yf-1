/*
 * YF
 * yf-animation.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_ANIMATION_H
#define YF_YF_ANIMATION_H

#include "yf/com/yf-defs.h"

#include "yf-node.h"
#include "yf-vector.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining keyframe animation.
 */
typedef struct YF_animation_o *YF_animation;

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
typedef struct {
    float *timeline;
    unsigned n;
} YF_kfinput;

/**
 * Type defining output values for an animation.
 */
typedef struct {
    int kfprop;
    union {
        YF_vec3 *t;
        YF_vec4 *r;
        YF_vec3 *s;
    };
    unsigned n;
} YF_kfoutput;

/**
 * Type defining an animation action.
 */
typedef struct {
    int kferp;
    /* Indices in the 'YF_kfinput' and 'YF_kfoutput' arrays used to
       initialize an animation. The number of samples must match. */
    unsigned in_i;
    unsigned out_i;
} YF_kfaction;

/**
 * Initializes a new animation.
 *
 * @param inputs: The 'YF_kfinput' array containing all keyframe inputs
 *  needed by 'actions'.
 * @param input_n: The length of 'inputs'. Must be at least one.
 * @param outputs: The 'YF_kfoutput' array containing all keyframe outputs
 *  needed by 'actions'.
 * @param output_n: The length of 'outputs'. Must be at least one.
 * @param actions: The 'YF_kfaction' array referencing 'inputs' and 'outputs'
 *  to define the keyframe animation.
 * @param action_n: The length of 'actions'. Must be at least one.
 * @return: On success, returns a new animation. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_animation yf_animation_init(const YF_kfinput *inputs, unsigned input_n,
                               const YF_kfoutput *outputs, unsigned output_n,
                               const YF_kfaction *actions, unsigned action_n);

/**
 * Sets the target of an animation's action.
 *
 * @param anim: The animation.
 * @param action: The index in the 'YF_kfaction' array of 'anim' indicating
 *  the action whose target is to be set.
 * @param target: The node to set as target. Can be 'NULL'.
 */
void yf_animation_settarget(YF_animation anim, unsigned action,
                            YF_node target);

/**
 * Gets the inputs of an animation.
 *
 * @param anim: The animation.
 * @param n: The destination for the array size.
 * @return: The 'YF_kfinput' array of 'anim'.
 */
const YF_kfinput *yf_animation_getins(YF_animation anim, unsigned *n);

/**
 * Gets the outputs of an animation.
 *
 * @param anim: The animation.
 * @param n: The destination for the array size.
 * @return: The 'YF_kfoutput' array of 'anim'.
 */
const YF_kfoutput *yf_animation_getouts(YF_animation anim, unsigned *n);

/**
 * Gets the actions of an animation.
 *
 * @param anim: The animation.
 * @param n: The destination for the array size.
 * @return: The 'YF_kfaction' array of 'anim'.
 */
const YF_kfaction *yf_animation_getacts(YF_animation anim, unsigned *n);

/**
 * Deinitializes an animation.
 *
 * @param anim: The animation to deinitialize. Can be 'NULL'.
 */
void yf_animation_deinit(YF_animation anim);

YF_DECLS_END

#endif /* YF_YF_ANIMATION_H */
