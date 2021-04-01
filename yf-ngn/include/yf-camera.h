/*
 * YF
 * yf-camera.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_CAMERA_H
#define YF_YF_CAMERA_H

#include <yf/com/yf-defs.h>

#include "yf-matrix.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a camera.
 */
typedef struct YF_camera_o *YF_camera;

/**
 * Initializes a new camera.
 *
 * The positive y-axis is used as the fixed world 'up' vector (0, 1, 0).
 *
 * NOTE: The camera's origin and target must differ, otherwise a division
 * by zero will occur.
 *
 * @param origin: The location to place the camera at.
 * @param target: The location to point the camera towards.
 * @param aspect: The aspect ratio to use.
 * @return: On success, returns a new camera. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_camera yf_camera_init(const YF_vec3 origin, const YF_vec3 target,
    YF_float aspect);

/**
 * Places the camera at a given position.
 *
 * @param cam: The camera.
 * @param pos: The position to place thre camera at.
 */
void yf_camera_place(YF_camera cam, const YF_vec3 pos);

/**
 * Points the camera towards a given position.
 *
 * @param cam: The camera.
 * @param pos: The position to point the camera towards.
 */
void yf_camera_point(YF_camera cam, const YF_vec3 pos);

/**
 * Moves the camera forward.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_movef(YF_camera cam, YF_float d);

/**
 * Moves the camera backward.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_moveb(YF_camera cam, YF_float d);

/**
 * Moves the camera upward.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_moveu(YF_camera cam, YF_float d);

/**
 * Moves the camera downward.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_moved(YF_camera cam, YF_float d);

/**
 * Moves the camera sideways to the left.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_movel(YF_camera cam, YF_float d);

/**
 * Moves the camera sideways to the right.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_mover(YF_camera cam, YF_float d);

/**
 * Turns the camera up.
 *
 * @param cam: The camera.
 * @param d: The amount to turn.
 */
void yf_camera_turnu(YF_camera cam, YF_float d);

/**
 * Turns the camera down.
 *
 * @param cam: The camera.
 * @param d: The amount to turn.
 */
void yf_camera_turnd(YF_camera cam, YF_float d);

/**
 * Turns the camera to the left.
 *
 * @param cam: The camera.
 * @param d: The amount to turn.
 */
void yf_camera_turnl(YF_camera cam, YF_float d);

/**
 * Turns the camera to the right.
 *
 * @param cam: The camera.
 * @param d: The amount to turn.
 */
void yf_camera_turnr(YF_camera cam, YF_float d);

/**
 * Zooms the camera in.
 *
 * @param cam: The camera.
 * @param d: The amount to zoom.
 */
void yf_camera_zoomi(YF_camera cam, YF_float d);

/**
 * Zooms the camera out
 *
 * @param cam: The camera.
 * @param d: The amount to zoom.
 */
void yf_camera_zoomo(YF_camera cam, YF_float d);

/**
 * Gets the camera's view-projection matrix.
 *
 * @param cam: The camera.
 * @return: The current view-projection matrix of the camera.
 */
const YF_mat4 *yf_camera_getxform(YF_camera cam);

/**
 * Gets the camera's view matrix.
 *
 * @param cam: The camera.
 * @return: The current view matrix of the camera.
 */
const YF_mat4 *yf_camera_getview(YF_camera cam);

/**
 * Gets the camera's projection matrix.
 *
 * @param cam: The camera.
 * @return: The current projection matrix of the camera.
 */
const YF_mat4 *yf_camera_getproj(YF_camera cam);

/**
 * Adjusts the camera to a given aspect ratio.
 *
 * @param cam: The camera.
 * @param aspect: The new aspect ratio.
 */
void yf_camera_adjust(YF_camera cam, YF_float aspect);

/**
 * Deinitializes a camera.
 *
 * @param cam: The camera to deinitialize. Can be 'NULL'.
 */
void yf_camera_deinit(YF_camera cam);

YF_DECLS_END

#endif /* YF_YF_CAMERA_H */
