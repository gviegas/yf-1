/*
 * YF
 * yf-animation.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_ANIMATION_H
#define YF_YF_ANIMATION_H

#include "yf/com/yf-defs.h"

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
 * Initializes a new animation.
 *
 * @return: On success, returns a new animation. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_animation yf_animation_init(void);

/**
 * Deinitializes an animation.
 *
 * @param anim: The animation to deinitialize. Can be 'NULL'.
 */
void yf_animation_deinit(YF_animation anim);

YF_DECLS_END

#endif /* YF_YF_ANIMATION_H */
