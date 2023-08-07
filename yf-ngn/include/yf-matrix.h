/*
 * YF
 * yf-matrix.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_MATRIX_H
#define YF_YF_MATRIX_H

#include "yf/com/yf-defs.h"

#include "yf-vector.h"

YF_DECLS_BEGIN

/**
 * Quadratic matrices.
 */
typedef float yf_mat2_t[2*2];
typedef float yf_mat3_t[3*3];
typedef float yf_mat4_t[4*4];

/**
 * Makes a given matrix an identity matrix.
 *
 * @param m: The matrix.
 */
void yf_mat2_iden(yf_mat2_t m);
void yf_mat3_iden(yf_mat3_t m);
void yf_mat4_iden(yf_mat4_t m);

/**
 * Sets all components of a matrix to a given scalar.
 *
 * @param m: The matrix.
 * @param s: The scalar.
 */
void yf_mat2_set(yf_mat2_t m, float s);
void yf_mat3_set(yf_mat3_t m, float s);
void yf_mat4_set(yf_mat4_t m, float s);

/**
 * Copies one matrix to another.
 *
 * @param dst: The destination matrix.
 * @param m: The source matrix.
 */
void yf_mat2_copy(yf_mat2_t dst, const yf_mat2_t m);
void yf_mat3_copy(yf_mat3_t dst, const yf_mat3_t m);
void yf_mat4_copy(yf_mat4_t dst, const yf_mat4_t m);

/**
 * Computes the transpose of a matrix.
 *
 * @param dst: The destination matrix.
 * @param m: The source matrix.
 */
void yf_mat2_xpose(yf_mat2_t dst, const yf_mat2_t m);
void yf_mat3_xpose(yf_mat3_t dst, const yf_mat3_t m);
void yf_mat4_xpose(yf_mat4_t dst, const yf_mat4_t m);

/**
 * Subtracts two matrices.
 *
 * @param dst: The destination matrix.
 * @param a: The first matrix.
 * @param b: The second matrix.
 */
void yf_mat2_sub(yf_mat2_t dst, const yf_mat2_t a, const yf_mat2_t b);
void yf_mat3_sub(yf_mat3_t dst, const yf_mat3_t a, const yf_mat3_t b);
void yf_mat4_sub(yf_mat4_t dst, const yf_mat4_t a, const yf_mat4_t b);

/**
 * Adds two matrices.
 *
 * @param dst: The destination matrix.
 * @param a: The first matrix.
 * @param b: The second matrix.
 */
void yf_mat2_add(yf_mat2_t dst, const yf_mat2_t a, const yf_mat2_t b);
void yf_mat3_add(yf_mat3_t dst, const yf_mat3_t a, const yf_mat3_t b);
void yf_mat4_add(yf_mat4_t dst, const yf_mat4_t a, const yf_mat4_t b);

/**
 * Multiplies two matrices.
 *
 * @param dst: The destination matrix.
 * @param a: The first matrix.
 * @param b: The second matrix.
 */
void yf_mat2_mul(yf_mat2_t dst, const yf_mat2_t a, const yf_mat2_t b);
void yf_mat3_mul(yf_mat3_t dst, const yf_mat3_t a, const yf_mat3_t b);
void yf_mat4_mul(yf_mat4_t dst, const yf_mat4_t a, const yf_mat4_t b);

/**
 * Multiplies a matrix and a vector.
 *
 * @param dst: The destination vector.
 * @param m: The matrix.
 * @param v: The vector.
 */
void yf_mat2_mulv(yf_vec2_t dst, const yf_mat2_t m, const yf_vec2_t v);
void yf_mat3_mulv(yf_vec3_t dst, const yf_mat3_t m, const yf_vec3_t v);
void yf_mat4_mulv(yf_vec4_t dst, const yf_mat4_t m, const yf_vec4_t v);

/**
 * Computes the inverse of a matrix.
 *
 * @param dst: The destination matrix.
 * @param m: The source matrix.
 */
void yf_mat2_inv(yf_mat2_t dst, const yf_mat2_t m);
void yf_mat3_inv(yf_mat3_t dst, const yf_mat3_t m);
void yf_mat4_inv(yf_mat4_t dst, const yf_mat4_t m);

/**
 * Computes a rotation matrix for the x-, y- or z-axis.
 *
 * @param m: The destination matrix.
 * @param angle: The rotation angle, in radians.
 */
void yf_mat3_rotx(yf_mat3_t m, float angle);
void yf_mat3_roty(yf_mat3_t m, float angle);
void yf_mat3_rotz(yf_mat3_t m, float angle);
void yf_mat4_rotx(yf_mat4_t m, float angle);
void yf_mat4_roty(yf_mat4_t m, float angle);
void yf_mat4_rotz(yf_mat4_t m, float angle);

/**
 * Computes a rotation matrix for a given axis.
 *
 * @param m: The destination matrix.
 * @param angle: The rotation angle, in radians.
 * @param axis: The rotation axis.
 */
void yf_mat3_rot(yf_mat3_t m, float angle, const yf_vec3_t axis);
void yf_mat4_rot(yf_mat4_t m, float angle, const yf_vec3_t axis);

/**
 * Computes a rotation matrix from a quaternion rotation.
 *
 * @param m: The destination matrix.
 * @param q: The quaternion rotation.
 */
void yf_mat3_rotq(yf_mat3_t m, const yf_vec4_t q);
void yf_mat4_rotq(yf_mat4_t m, const yf_vec4_t q);

/**
 * Computes a scaling matrix.
 *
 * @param m: The destination matrix.
 * @param sx: The x-axis scale.
 * @param sy: The y-axis scale.
 * @param sz: The z-axis scale.
 */
void yf_mat3_scale(yf_mat3_t m, float sx, float sy, float sz);
void yf_mat4_scale(yf_mat4_t m, float sx, float sy, float sz);

/**
 * Computes a translation matrix.
 *
 * @param m: The destination matrix.
 * @param tx: The x-axis translation.
 * @param ty: The y-axis translation.
 * @param tz: The z-axis translation.
 */
void yf_mat4_xlate(yf_mat4_t m, float tx, float ty, float tz);

/**
 * Computes a view matrix.
 *
 * @param m: The destination matrix.
 * @param eye: The eye (origin) vector.
 * @param center: The center (target) vector.
 * @param up: The up vector.
 */
void yf_mat4_lookat(yf_mat4_t m, const yf_vec3_t center, const yf_vec3_t eye,
                    const yf_vec3_t up);

/**
 * Computes a finite perspective projection matrix.
 *
 * @param m: The destination matrix.
 * @param yfov: The vertical field of view.
 * @param aspect: The aspect ratio of the field of view.
 * @param znear: The distance to the near clipping plane.
 * @param zfar: The distance to the far clipping plane.
 */
void yf_mat4_persp(yf_mat4_t m, float yfov, float aspect, float znear,
                   float zfar);

/**
 * Computes an infinite perspective projection matrix.
 *
 * @param m: The destination matrix.
 * @param yfov: The vertical field of view.
 * @param aspect: The aspect ratio of the field of view.
 * @param znear: The distance to the near clipping plane.
 */
void yf_mat4_infpersp(yf_mat4_t m, float yfov, float aspect, float znear);

/**
 * Computes an orthographic projection matrix.
 *
 * @param m: The destination matrix.
 * @param xmag: The horizontal magnification of the view.
 * @param ymag: The vertical magnification of the view.
 * @param znear: The distance to the near clipping plane.
 * @param zfar: The distance to the far clipping plane.
 */
void yf_mat4_ortho(yf_mat4_t m, float xmag, float ymag, float znear,
                   float zfar);

YF_DECLS_END

#endif /* YF_YF_MATRIX_H */
