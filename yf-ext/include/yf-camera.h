/*
 * YF
 * yf-camera.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_CAMERA_H
#define YF_YF_CAMERA_H

#include <yf/com/yf-defs.h>

#include "yf-matrix.h"

YF_DECLS_BEGIN

/* Opaque type defining a camera. */
typedef struct YF_camera_o *YF_camera;

/* Initializes a new camera. */
YF_camera yf_camera_init(
  const YF_vec3 origin,
  const YF_vec3 target,
  YF_float aspect);

/* Places the camera at a given position. */
void yf_camera_place(YF_camera cam, const YF_vec3 pos);

/* Points the camera towards a given position. */
void yf_camera_point(YF_camera cam, const YF_vec3 pos);

/* Moves the camera forward. */
void yf_camera_movef(YF_camera cam, YF_float d);

/* Moves the camera backward. */
void yf_camera_moveb(YF_camera cam, YF_float d);

/* Moves the camera upward. */
void yf_camera_moveu(YF_camera cam, YF_float d);

/* Moves the camera downward. */
void yf_camera_moved(YF_camera cam, YF_float d);

/* Moves the camera sideways to the left. */
void yf_camera_movel(YF_camera cam, YF_float d);

/* Moves the camera sideways to the right. */
void yf_camera_mover(YF_camera cam, YF_float d);

/* Turns the camera up. */
void yf_camera_turnu(YF_camera cam, YF_float d);

/* Turns the camera down. */
void yf_camera_turnd(YF_camera cam, YF_float d);

/* Turns the camera to the left. */
void yf_camera_turnl(YF_camera cam, YF_float d);

/* Turns the camera to the right. */
void yf_camera_turnr(YF_camera cam, YF_float d);

/* Zooms the camera in. */
void yf_camera_zoomi(YF_camera cam, YF_float d);

/* Zooms the camera out. */
void yf_camera_zoomo(YF_camera cam, YF_float d);

/* Gets the camera's view-projection matrix. */
const YF_mat4 *yf_camera_getxform(YF_camera cam);

/* Gets the camera's view matrix. */
const YF_mat4 *yf_camera_getview(YF_camera cam);

/* Gets the camera's projection matrix. */
const YF_mat4 *yf_camera_getproj(YF_camera cam);

/* Adjusts the camera to a given aspect ratio. */
void yf_camera_adjust(YF_camera cam, YF_float aspect);

/* Deinitializes a camera. */
void yf_camera_deinit(YF_camera cam);

YF_DECLS_END

#endif /* YF_YF_CAMERA_H */
