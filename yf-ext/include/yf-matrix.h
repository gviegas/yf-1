/*
 * YF
 * yf-matrix.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_MATRIX_H
#define YF_YF_MATRIX_H

#include "yf-common.h"
#include "yf-vector.h"

YF_DECLS_BEGIN

/* Quadratic matrices. */
typedef YF_float YF_mat2[4];
typedef YF_float YF_mat3[9];
typedef YF_float YF_mat4[16];

/* Makes a given matrix an identity matrix. */
void yf_mat2_iden(YF_mat2 m);
void yf_mat3_iden(YF_mat3 m);
void yf_mat4_iden(YF_mat4 m);

/* Sets all components of a matrix to a given scalar. */
void yf_mat2_set(YF_mat2 m, YF_float s);
void yf_mat3_set(YF_mat3 m, YF_float s);
void yf_mat4_set(YF_mat4 m, YF_float s);

/* Copies one matrix to another. */
void yf_mat2_copy(YF_mat2 dst, const YF_mat2 m);
void yf_mat3_copy(YF_mat3 dst, const YF_mat3 m);
void yf_mat4_copy(YF_mat4 dst, const YF_mat4 m);

/* Computes the transpose of a matrix. */
void yf_mat2_xpose(YF_mat2 dst, const YF_mat2 m);
void yf_mat3_xpose(YF_mat3 dst, const YF_mat3 m);
void yf_mat4_xpose(YF_mat4 dst, const YF_mat4 m);

/* Subtracts two matrices. */
void yf_mat2_sub(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b);
void yf_mat3_sub(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b);
void yf_mat4_sub(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b);

/* Adds two matrices. */
void yf_mat2_add(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b);
void yf_mat3_add(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b);
void yf_mat4_add(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b);

/* Multiplies two matrices. */
void yf_mat2_mul(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b);
void yf_mat3_mul(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b);
void yf_mat4_mul(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b);

/* Multiplies a matrix and a vector. */
void yf_mat2_mulv(YF_vec2 dst, const YF_mat2 m, const YF_vec2 v);
void yf_mat3_mulv(YF_vec3 dst, const YF_mat3 m, const YF_vec3 v);
void yf_mat4_mulv(YF_vec4 dst, const YF_mat4 m, const YF_vec4 v);

/* Computes the inverse of a matrix. */
void yf_mat2_inv(YF_mat2 dst, const YF_mat2 m);
void yf_mat3_inv(YF_mat3 dst, const YF_mat3 m);
void yf_mat4_inv(YF_mat4 dst, const YF_mat4 m);

/* Computes a rotation matrix for a given axis. */
void yf_mat3_rot(YF_mat3 m, YF_float angle, const YF_vec3 axis);
void yf_mat4_rot(YF_mat4 m, YF_float angle, const YF_vec3 axis);

/* Computes a rotation matrix for x, y or z axis. */
void yf_mat3_rotx(YF_mat3 m, YF_float angle);
void yf_mat3_roty(YF_mat3 m, YF_float angle);
void yf_mat3_rotz(YF_mat3 m, YF_float angle);
void yf_mat4_rotx(YF_mat4 m, YF_float angle);
void yf_mat4_roty(YF_mat4 m, YF_float angle);
void yf_mat4_rotz(YF_mat4 m, YF_float angle);

/* Computes a scaling matrix. */
void yf_mat3_scale(YF_mat3 m, YF_float sx, YF_float sy, YF_float sz);
void yf_mat4_scale(YF_mat4 m, YF_float sx, YF_float sy, YF_float sz);

/* Computes a translation matrix. */
void yf_mat4_xlate(YF_mat4 m, YF_float tx, YF_float ty, YF_float tz);

/* Computes a 'Look At' (camera/view) matrix. */
void yf_mat4_lookat(
  YF_mat4 m,
  const YF_vec3 eye,
  const YF_vec3 center,
  const YF_vec3 up);

/* Computes a perspective projection matrix. */
void yf_mat4_persp(
  YF_mat4 m,
  YF_float yfov,
  YF_float aspect,
  YF_float znear,
  YF_float zfar);

/* Computes an 'Ortho' (non-perspective) matrix. */
void yf_mat4_ortho(
  YF_mat4 m,
  YF_float left,
  YF_float right,
  YF_float top,
  YF_float bottom,
  YF_float znear,
  YF_float zfar);

YF_DECLS_END

#endif /* YF_YF_MATRIX_H */
