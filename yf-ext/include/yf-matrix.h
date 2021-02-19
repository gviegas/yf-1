/*
 * YF
 * yf-matrix.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_MATRIX_H
#define YF_YF_MATRIX_H

#include <yf/com/yf-defs.h>
#include <yf/com/yf-types.h>

#include "yf-vector.h"

YF_DECLS_BEGIN

/**
 * Quadratic matrices.
 */
typedef YF_float YF_mat2[4];
typedef YF_float YF_mat3[9];
typedef YF_float YF_mat4[16];

/**
 * Makes a given matrix an identity matrix.
 *
 * @param m: The matrix.
 */
void yf_mat2_iden(YF_mat2 m);
void yf_mat3_iden(YF_mat3 m);
void yf_mat4_iden(YF_mat4 m);

/**
 * Sets all components of a matrix to a given scalar.
 *
 * @param m: The matrix.
 * @param s: The scalar.
 */
void yf_mat2_set(YF_mat2 m, YF_float s);
void yf_mat3_set(YF_mat3 m, YF_float s);
void yf_mat4_set(YF_mat4 m, YF_float s);

/**
 * Copies one matrix to another.
 *
 * @param dst: The destination matrix.
 * @param m: The source matrix.
 */
void yf_mat2_copy(YF_mat2 dst, const YF_mat2 m);
void yf_mat3_copy(YF_mat3 dst, const YF_mat3 m);
void yf_mat4_copy(YF_mat4 dst, const YF_mat4 m);

/**
 * Computes the transpose of a matrix.
 *
 * @param dst: The destination matrix.
 * @param m: The source matrix.
 */
void yf_mat2_xpose(YF_mat2 dst, const YF_mat2 m);
void yf_mat3_xpose(YF_mat3 dst, const YF_mat3 m);
void yf_mat4_xpose(YF_mat4 dst, const YF_mat4 m);

/**
 * Subtracts two matrices.
 *
 * @param dst: The destination matrix.
 * @param a: The first matrix.
 * @param b: The second matrix.
 */
void yf_mat2_sub(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b);
void yf_mat3_sub(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b);
void yf_mat4_sub(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b);

/**
 * Adds two matrices.
 *
 * @param dst: The destination matrix.
 * @param a: The first matrix.
 * @param b: The second matrix.
 */
void yf_mat2_add(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b);
void yf_mat3_add(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b);
void yf_mat4_add(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b);

/**
 * Multiplies two matrices.
 *
 * @param dst: The destination matrix.
 * @param a: The first matrix.
 * @param b: The second matrix.
 */
void yf_mat2_mul(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b);
void yf_mat3_mul(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b);
void yf_mat4_mul(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b);

/**
 * Multiplies a matrix and a vector.
 *
 * @param dst: The destination vector.
 * @param m: The matrix.
 * @param v: The vector.
 */
void yf_mat2_mulv(YF_vec2 dst, const YF_mat2 m, const YF_vec2 v);
void yf_mat3_mulv(YF_vec3 dst, const YF_mat3 m, const YF_vec3 v);
void yf_mat4_mulv(YF_vec4 dst, const YF_mat4 m, const YF_vec4 v);

/**
 * Computes the inverse of a matrix.
 *
 * @param dst: The destination matrix.
 * @param m: The source matrix.
 */
void yf_mat2_inv(YF_mat2 dst, const YF_mat2 m);
void yf_mat3_inv(YF_mat3 dst, const YF_mat3 m);
void yf_mat4_inv(YF_mat4 dst, const YF_mat4 m);

/**
 * Computes a rotation matrix for a given axis.
 *
 * @param m: The destination matrix.
 * @param angle: The rotation angle, in radians.
 * @param axis: The rotation axis.
 */
void yf_mat3_rot(YF_mat3 m, YF_float angle, const YF_vec3 axis);
void yf_mat4_rot(YF_mat4 m, YF_float angle, const YF_vec3 axis);

/**
 * Computes a rotation matrix for the x-, y- or z-axis.
 *
 * @param m: The destination matrix.
 * @param angle: The rotation angle, in radians.
 */
void yf_mat3_rotx(YF_mat3 m, YF_float angle);
void yf_mat3_roty(YF_mat3 m, YF_float angle);
void yf_mat3_rotz(YF_mat3 m, YF_float angle);
void yf_mat4_rotx(YF_mat4 m, YF_float angle);
void yf_mat4_roty(YF_mat4 m, YF_float angle);
void yf_mat4_rotz(YF_mat4 m, YF_float angle);

/**
 * Computes a rotation matrix from a quaternion rotation.
 *
 * @param m: The destination matrix.
 * @param q: The quaternion rotation.
 */
void yf_mat3_rotq(YF_mat3 m, const YF_vec4 q);
void yf_mat4_rotq(YF_mat4 m, const YF_vec4 q);

/**
 * Computes a scaling matrix.
 *
 * @param m: The destination matrix.
 * @param sx: The x-axis scale.
 * @param sy: The y-axis scale.
 * @param sz: The z-axis scale.
 */
void yf_mat3_scale(YF_mat3 m, YF_float sx, YF_float sy, YF_float sz);
void yf_mat4_scale(YF_mat4 m, YF_float sx, YF_float sy, YF_float sz);

/**
 * Computes a translation matrix.
 *
 * @param m: The destination matrix.
 * @param tx: The x-axis translation.
 * @param ty: The y-axis translation.
 * @param tz: The z-axis translation.
 */
void yf_mat4_xlate(YF_mat4 m, YF_float tx, YF_float ty, YF_float tz);

/**
 * Computes a 'Look At' (camera/view) matrix.
 *
 * @param m: The destination matrix.
 * @param eye: The eye (origin) vector.
 * @param center: The center (target) vector.
 * @param up: The up vector.
 */
void yf_mat4_lookat(YF_mat4 m, const YF_vec3 eye, const YF_vec3 center,
    const YF_vec3 up);

/**
 * Computes a perspective projection matrix.
 *
 * @param m: The destination matrix.
 * @param yfov: The y field of view.
 * @param aspect: The aspect ratio.
 * @param znear: The z near (min depth) value.
 * @param zfar: The z far (max depth) value.
 */
void yf_mat4_persp(YF_mat4 m, YF_float yfov, YF_float aspect,
    YF_float znear, YF_float zfar);

/**
 * Computes an 'Ortho' (non-perspective) matrix.
 *
 * @param m: The destination matrix.
 * @param left: The left side value.
 * @param right: The right side value.
 * @param top: The top value.
 * @param bottom: The bottom value.
 * @param znear: The z near (min depth) value.
 * @param zfar: The z far (max depth) value.
 */
void yf_mat4_ortho(YF_mat4 m, YF_float left, YF_float right,
    YF_float top, YF_float bottom, YF_float znear, YF_float zfar);

YF_DECLS_END

#endif /* YF_YF_MATRIX_H */
