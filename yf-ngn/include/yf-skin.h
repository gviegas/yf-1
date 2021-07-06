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

/* Opaque type defining an instance of a skin's joint hierarchy. */
typedef struct YF_skeleton_o *YF_skeleton;

/* Type describing a joint for skinning. */
typedef struct {
    YF_mat4 xform;
    YF_mat4 ibm;
    char name[32];
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
 * Makes a new skeleton from a given skin.
 *
 * @param skin: The skin.
 * @return: On success, returns a new skeleton. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_skeleton yf_skin_makeskel(YF_skin skin);

/**
 * Unmakes a skin's skeleton.
 *
 * @param skin: The skin that produced 'skel'.
 * @param skel: The skeleton to unmake.
 */
void yf_skin_unmkskel(YF_skin skin, YF_skeleton skel);

/**
 * Deinitializes a skin.
 *
 * @param skin: The skin to deinitialize. Can be 'NULL'.
 */
void yf_skin_deinit(YF_skin skin);

YF_DECLS_END

#endif /* YF_YF_SKIN_H */
