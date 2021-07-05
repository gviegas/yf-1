/*
 * YF
 * yf-skin.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_SKIN_H
#define YF_YF_SKIN_H

#include "yf/com/yf-defs.h"

#include "yf-matrix.h"

YF_DECLS_BEGIN

/* Opaque type defining a skin. */
typedef struct YF_skin_o *YF_skin;

/* Type defining a joint. */
typedef struct {
    YF_mat4 xform;
    YF_mat4 ibm;
    /* Index of the joint's parent in the array used to initialize a skin.
       A negative value indicates that the joint has no parent. */
    long pnt_i;
} YF_joint;

/**
 * Initializes a new skin.
 *
 * @param jnts: The array of 'YF_joint' describing the skin's skeleton.
 * @param jnt_n: The size of the 'jnts' array. Must be greater than zero.
 * @return: On success, returns a new skin. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_skin yf_skin_init(const YF_joint *jnts, unsigned jnt_n);

/**
 * Deinitializes a skin.
 *
 * @param skin: The skin to deinitialize. Can be 'NULL'.
 */
void yf_skin_deinit(YF_skin skin);

YF_DECLS_END

#endif /* YF_YF_SKIN_H */
