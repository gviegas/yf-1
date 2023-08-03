/*
 * YF
 * yf-camera.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_CAMERA_H
#define YF_YF_CAMERA_H

#include "yf/com/yf-defs.h"

#include "yf-matrix.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a camera.
 */
typedef struct yf_camera yf_camera_t;

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
yf_camera_t *yf_camera_init(const yf_vec3_t origin, const yf_vec3_t target,
                            float aspect);

/**
 * Places the camera at a given position.
 *
 * @param cam: The camera.
 * @param pos: The position to place the camera at.
 */
void yf_camera_place(yf_camera_t *cam, const yf_vec3_t pos);

/**
 * Points the camera towards a given position.
 *
 * @param cam: The camera.
 * @param pos: The position to point the camera towards.
 */
void yf_camera_point(yf_camera_t *cam, const yf_vec3_t pos);

/**
 * Moves the camera forward.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_movef(yf_camera_t *cam, float d);

/**
 * Moves the camera backward.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_moveb(yf_camera_t *cam, float d);

/**
 * Moves the camera upward.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_moveu(yf_camera_t *cam, float d);

/**
 * Moves the camera downward.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_moved(yf_camera_t *cam, float d);

/**
 * Moves the camera sideways to the left.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_movel(yf_camera_t *cam, float d);

/**
 * Moves the camera sideways to the right.
 *
 * @param cam: The camera.
 * @param d: The amount to move.
 */
void yf_camera_mover(yf_camera_t *cam, float d);

/**
 * Turns the camera up.
 *
 * @param cam: The camera.
 * @param d: The amount to turn.
 */
void yf_camera_turnu(yf_camera_t *cam, float d);

/**
 * Turns the camera down.
 *
 * @param cam: The camera.
 * @param d: The amount to turn.
 */
void yf_camera_turnd(yf_camera_t *cam, float d);

/**
 * Turns the camera to the left.
 *
 * @param cam: The camera.
 * @param d: The amount to turn.
 */
void yf_camera_turnl(yf_camera_t *cam, float d);

/**
 * Turns the camera to the right.
 *
 * @param cam: The camera.
 * @param d: The amount to turn.
 */
void yf_camera_turnr(yf_camera_t *cam, float d);

/**
 * Zooms the camera in.
 *
 * @param cam: The camera.
 * @param d: The amount to zoom.
 */
void yf_camera_zoomi(yf_camera_t *cam, float d);

/**
 * Zooms the camera out.
 *
 * @param cam: The camera.
 * @param d: The amount to zoom.
 */
void yf_camera_zoomo(yf_camera_t *cam, float d);

/**
 * Gets the camera's view-projection matrix.
 *
 * @param cam: The camera.
 * @return: The current view-projection matrix of the camera.
 */
const yf_mat4_t *yf_camera_getxform(yf_camera_t *cam);

/**
 * Gets the camera's view matrix.
 *
 * @param cam: The camera.
 * @return: The current view matrix of the camera.
 */
const yf_mat4_t *yf_camera_getview(yf_camera_t *cam);

/**
 * Gets the camera's projection matrix.
 *
 * @param cam: The camera.
 * @return: The current projection matrix of the camera.
 */
const yf_mat4_t *yf_camera_getproj(yf_camera_t *cam);

/**
 * Adjusts the camera to a given aspect ratio.
 *
 * @param cam: The camera.
 * @param aspect: The new aspect ratio.
 */
void yf_camera_adjust(yf_camera_t *cam, float aspect);

/**
 * Deinitializes a camera.
 *
 * @param cam: The camera to deinitialize. Can be 'NULL'.
 */
void yf_camera_deinit(yf_camera_t *cam);

YF_DECLS_END

#endif /* YF_YF_CAMERA_H */
