/*
 * YF
 * yf-skin.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_SKIN_H
#define YF_YF_SKIN_H

#include "yf/com/yf-defs.h"

#include "yf-node.h"
#include "yf-matrix.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a skin.
 */
typedef struct YF_skin_o *YF_skin;

/**
 * Opaque type defining an instance of a skin's joint hierarchy.
 */
typedef struct YF_skeleton_o *YF_skeleton;

/**
 * Type describing a joint for skinning.
 */
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
 * Gets the joint descriptions of a skin.
 *
 * @param skin: The skin.
 * @param jnt_n: The destination for the size of the 'YF_joint' array.
 * @return: The skin's 'YF_joint' array.
 */
const YF_joint *yf_skin_getjnts(YF_skin skin, unsigned *jnt_n);

/**
 * Executes a given function for each instantiated skeleton of a skin.
 *
 * This function completes after executing 'callb' for every instantiated
 * skeleton of 'skin' or when 'callb' returns a non-zero value.
 *
 * One must not make nor unmake skeletons using 'skin' until this function
 * completes.
 *
 * @param skin: The skin.
 * @param callb: The callback to execute for each instance.
 * @param arg: The generic argument to pass on 'callb' calls. Can be 'NULL'.
 */
void yf_skin_each(YF_skin skin, int (*callb)(YF_skeleton skel, void *arg),
                  void *arg);

/**
 * Makes a new skeleton from a given skin.
 *
 * @param skin: The skin.
 * @param nodes: Optional array of nodes for the skeleton. When provided,
 *  this array must contain all joint nodes plus one additional node in the
 *  last position, which will be used as skeleton root. The caller is then
 *  responsible for constructing the skeleton hierarchy, for setting its
 *  properties and for managing the nodes' lifetime.
 * @return: On success, returns a new skeleton. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_skeleton yf_skin_makeskel(YF_skin skin, const YF_node *nodes);

/**
 * Gets the root node of a skin's skeleton.
 *
 * @param skin: The skin.
 * @param skel: The skeleton.
 * @return: The skeleton's root node.
 */
YF_node yf_skin_getnode(YF_skin skin, YF_skeleton skel);

/**
 * Gets a joint node of a skin's skeleton.
 *
 * @param skin: The skin.
 * @param skel: The skeleton.
 * @param index: The index of the joint in the 'jnts' array used to initialize
 *  the skin.
 * @return: The skeleton's joint node for 'index'.
 */
YF_node yf_skin_getjntnode(YF_skin skin, YF_skeleton skel, unsigned index);

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
 * Deinitializing a skin also unmakes all of its instantiated skeletons.
 *
 * @param skin: The skin to deinitialize. Can be 'NULL'.
 */
void yf_skin_deinit(YF_skin skin);

YF_DECLS_END

#endif /* YF_YF_SKIN_H */
