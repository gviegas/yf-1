/*
 * YF
 * yf-vector.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_VECTOR_H
#define YF_YF_VECTOR_H

#include "yf/com/yf-defs.h"

YF_DECLS_BEGIN

/**
 * 2-, 3- and 4-component vectors.
 */
typedef float yf_vec2_t[2];
typedef float yf_vec3_t[3];
typedef float yf_vec4_t[4];

/**
 * Checks whether or not a given vector is the zero vector.
 *
 * @param v: The vector.
 * @return: If 'v' is the zero vector, returns a non-zero value. Otherwise,
 *  zero is returned.
 */
int yf_vec2_iszero(const yf_vec2_t v);
int yf_vec3_iszero(const yf_vec3_t v);
int yf_vec4_iszero(const yf_vec4_t v);

/**
 * Checks whether or not a given vector is equal to another.
 *
 * @param a: The vector.
 * @param b: The other vector.
 * @return: If 'a' and 'b' are equal, returns a non-zero value. Otherwise,
 *  zero is returned.
 */
int yf_vec2_iseq(const yf_vec2_t a, const yf_vec2_t b);
int yf_vec3_iseq(const yf_vec3_t a, const yf_vec3_t b);
int yf_vec4_iseq(const yf_vec4_t a, const yf_vec4_t b);

/**
 * Sets all vector components to a given scalar.
 *
 * @param v: The vector.
 * @param s: The scalar.
 */
void yf_vec2_set(yf_vec2_t v, float s);
void yf_vec3_set(yf_vec3_t v, float s);
void yf_vec4_set(yf_vec4_t v, float s);

/**
 * Copies one vector to another.
 *
 * @param dst: The destination vector.
 * @param v: The source vector.
 */
void yf_vec2_copy(yf_vec2_t dst, const yf_vec2_t v);
void yf_vec3_copy(yf_vec3_t dst, const yf_vec3_t v);
void yf_vec4_copy(yf_vec4_t dst, const yf_vec4_t v);

/**
 * Subtracts two vectors.
 *
 * @param dst: The destination vector.
 * @param a: The first vector.
 * @param b: The second vector.
 */
void yf_vec2_sub(yf_vec2_t dst, const yf_vec2_t a, const yf_vec2_t b);
void yf_vec3_sub(yf_vec3_t dst, const yf_vec3_t a, const yf_vec3_t b);
void yf_vec4_sub(yf_vec4_t dst, const yf_vec4_t a, const yf_vec4_t b);

/**
 * Subtracts a given vector from a destination vector.
 *
 * @param dst: The destination vector.
 * @param v: The vector to subtract.
 */
void yf_vec2_subi(yf_vec2_t dst, const yf_vec2_t v);
void yf_vec3_subi(yf_vec3_t dst, const yf_vec3_t v);
void yf_vec4_subi(yf_vec4_t dst, const yf_vec4_t v);

/**
 * Adds two vectors.
 *
 * @param dst: The destination vector.
 * @param a: The first vector.
 * @param b: The second vector.
 */
void yf_vec2_add(yf_vec2_t dst, const yf_vec2_t a, const yf_vec2_t b);
void yf_vec3_add(yf_vec3_t dst, const yf_vec3_t a, const yf_vec3_t b);
void yf_vec4_add(yf_vec4_t dst, const yf_vec4_t a, const yf_vec4_t b);

/**
 * Adds a given vector to a destination vector.
 *
 * @param dst: The destination vector.
 * @param v: The vector to add.
 */
void yf_vec2_addi(yf_vec2_t dst, const yf_vec2_t v);
void yf_vec3_addi(yf_vec3_t dst, const yf_vec3_t v);
void yf_vec4_addi(yf_vec4_t dst, const yf_vec4_t v);

/**
 * Multiplies a vector and a scalar.
 *
 * @param dst: The destination vector.
 * @param v: The vector.
 * @param s: The scalar.
 */
void yf_vec2_muls(yf_vec2_t dst, const yf_vec2_t v, float s);
void yf_vec3_muls(yf_vec3_t dst, const yf_vec3_t v, float s);
void yf_vec4_muls(yf_vec4_t dst, const yf_vec4_t v, float s);

/**
 * Multiplies a destination vector by a given scalar.
 *
 * @param dst: The destination vector.
 * @param s: The scalar.
 */
void yf_vec2_mulsi(yf_vec2_t dst, float s);
void yf_vec3_mulsi(yf_vec3_t dst, float s);
void yf_vec4_mulsi(yf_vec4_t dst, float s);

/**
 * Computes the dot product of two vectors.
 *
 * @param a: The first vector.
 * @param b: The second vector.
 * @return: The dot product.
 */
float yf_vec2_dot(const yf_vec2_t a, const yf_vec2_t b);
float yf_vec3_dot(const yf_vec3_t a, const yf_vec3_t b);
float yf_vec4_dot(const yf_vec4_t a, const yf_vec4_t b);

/**
 * Computes the length of a vector.
 *
 * @param v: The vector.
 * @return: The length.
 */
float yf_vec2_len(const yf_vec2_t v);
float yf_vec3_len(const yf_vec3_t v);
float yf_vec4_len(const yf_vec4_t v);

/**
 * Normalizes a vector.
 *
 * @param dst: The destination vector.
 * @param v: The source vector.
 */
void yf_vec2_norm(yf_vec2_t dst, const yf_vec2_t v);
void yf_vec3_norm(yf_vec3_t dst, const yf_vec3_t v);
void yf_vec4_norm(yf_vec4_t dst, const yf_vec4_t v);

/**
 * Normalizes a vector in-place.
 *
 * @param v: The vector.
 */
void yf_vec2_normi(yf_vec2_t v);
void yf_vec3_normi(yf_vec3_t v);
void yf_vec4_normi(yf_vec4_t v);

/**
 * Computes the cross product of two vectors.
 *
 * @param dst: The destination vector.
 * @param a: The first vector.
 * @param b: The second vector.
 */
void yf_vec3_cross(yf_vec3_t dst, const yf_vec3_t a, const yf_vec3_t b);
/* TODO: Remove; this makes no sense. */
void yf_vec4_cross(yf_vec4_t dst, const yf_vec4_t a, const yf_vec4_t b);

/**
 * Computes a quaternion rotation for the x-, y-, or z-axis.
 *
 * The quaternion is encoded in a 'vec4' as follows:
 *  (r, v) : (vec4[3], vec4[0-2])
 *
 * @param q: The destination quaternion.
 * @param angle: The rotation angle, in radians.
 */
void yf_vec4_rotqx(yf_vec4_t q, float angle);
void yf_vec4_rotqy(yf_vec4_t q, float angle);
void yf_vec4_rotqz(yf_vec4_t q, float angle);

/**
 * Computes a quaternion rotation for a given axis.
 *
 * The quaternion is encoded in a 'vec4' as follows:
 *  (r, v) : (vec4[3], vec4[0-2])
 *
 * @param q: The destination quaternion.
 * @param angle: The rotation angle, in radians.
 * @param axis: The rotation axis.
 */
void yf_vec4_rotq(yf_vec4_t q, float angle, const yf_vec3_t axis);

/**
 * Multiplies two quaternions.
 *
 * The quaternion is encoded in a 'vec4' as follows:
 *  (r, v) : (vec4[3], vec4[0-2])
 *
 * @param dst: The destination quaternion.
 * @param q1: The first quaternion.
 * @param q2: The second quaternion.
 */
void yf_vec4_mulq(yf_vec4_t dst, const yf_vec4_t q1, const yf_vec4_t q2);

/**
 * Multiplies a destination quaternion by another.
 *
 * The quaternion is encoded in a 'vec4' as follows:
 *  (r, v) : (vec4[3], vec4[0-2])
 *
 * @param dst: The destination quaternion.
 * @param q: The other quaternion.
 */
void yf_vec4_mulqi(yf_vec4_t dst, const yf_vec4_t q);

YF_DECLS_END

#endif /* YF_YF_VECTOR_H */
