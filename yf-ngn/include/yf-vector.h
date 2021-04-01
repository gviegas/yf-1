/*
 * YF
 * yf-vector.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_VECTOR_H
#define YF_YF_VECTOR_H

#include <yf/com/yf-defs.h>
#include <yf/com/yf-types.h>

YF_DECLS_BEGIN

/**
 * 2-, 3- and 4-component vectors.
 */
typedef YF_float YF_vec2[2];
typedef YF_float YF_vec3[3];
typedef YF_float YF_vec4[4];

/**
 * Checks whether or not a given vector is the zero vector.
 *
 * @param v: The vector.
 * @return: If 'v' is the zero vector, returns a non-zero value. Otherwise,
 *  zero is returned.
 */
int yf_vec2_iszero(const YF_vec2 v);
int yf_vec3_iszero(const YF_vec3 v);
int yf_vec4_iszero(const YF_vec4 v);

/**
 * Checks whether or not a given vector is equal to another.
 *
 * @param a: The vector.
 * @param b: The other vector.
 * @return: If 'a' and 'b' are equal, returns a non-zero value. Otherwise,
 *  zero is returned.
 */
int yf_vec2_iseq(const YF_vec2 a, const YF_vec2 b);
int yf_vec3_iseq(const YF_vec3 a, const YF_vec3 b);
int yf_vec4_iseq(const YF_vec4 a, const YF_vec4 b);

/**
 * Sets all vector components to a given scalar.
 *
 * @param v: The vector.
 * @param s: The scalar.
 */
void yf_vec2_set(YF_vec2 v, YF_float s);
void yf_vec3_set(YF_vec3 v, YF_float s);
void yf_vec4_set(YF_vec4 v, YF_float s);

/**
 * Copies one vector to another.
 *
 * @param dst: The destination vector.
 * @param v: The source vector.
 */
void yf_vec2_copy(YF_vec2 dst, const YF_vec2 v);
void yf_vec3_copy(YF_vec3 dst, const YF_vec3 v);
void yf_vec4_copy(YF_vec4 dst, const YF_vec4 v);

/**
 * Subtracts two vectors.
 *
 * @param dst: The destination vector.
 * @param a: The first vector.
 * @param b: The second vector.
 */
void yf_vec2_sub(YF_vec2 dst, const YF_vec2 a, const YF_vec2 b);
void yf_vec3_sub(YF_vec3 dst, const YF_vec3 a, const YF_vec3 b);
void yf_vec4_sub(YF_vec4 dst, const YF_vec4 a, const YF_vec4 b);

/**
 * Subtracts a given vector from a destination vector.
 *
 * @param dst: The destination vector.
 * @param v: The vector to subtract.
 */
void yf_vec2_subi(YF_vec2 dst, const YF_vec2 v);
void yf_vec3_subi(YF_vec3 dst, const YF_vec3 v);
void yf_vec4_subi(YF_vec4 dst, const YF_vec4 v);

/**
 * Adds two vectors.
 *
 * @param dst: The destination vector.
 * @param a: The first vector.
 * @param b: The second vector.
 */
void yf_vec2_add(YF_vec2 dst, const YF_vec2 a, const YF_vec2 b);
void yf_vec3_add(YF_vec3 dst, const YF_vec3 a, const YF_vec3 b);
void yf_vec4_add(YF_vec4 dst, const YF_vec4 a, const YF_vec4 b);

/**
 * Adds a given vector to a destination vector.
 *
 * @param dst: The destination vector.
 * @param v: The vector to add.
 */
void yf_vec2_addi(YF_vec2 dst, const YF_vec2 v);
void yf_vec3_addi(YF_vec3 dst, const YF_vec3 v);
void yf_vec4_addi(YF_vec4 dst, const YF_vec4 v);

/**
 * Multiplies a vector and a scalar.
 *
 * @param dst: The destination vector.
 * @param v: The vector.
 * @param s: The scalar.
 */
void yf_vec2_muls(YF_vec2 dst, const YF_vec2 v, YF_float s);
void yf_vec3_muls(YF_vec3 dst, const YF_vec3 v, YF_float s);
void yf_vec4_muls(YF_vec4 dst, const YF_vec4 v, YF_float s);

/**
 * Multiplies a destination vector by a given scalar.
 *
 * @param dst: The destination vector.
 * @param s: The scalar.
 */
void yf_vec2_mulsi(YF_vec2 dst, YF_float s);
void yf_vec3_mulsi(YF_vec3 dst, YF_float s);
void yf_vec4_mulsi(YF_vec4 dst, YF_float s);

/**
 * Computes the dot product of two vectors.
 *
 * @param a: The first vector.
 * @param b: The second vector.
 * @return: The dot product.
 */
YF_float yf_vec2_dot(const YF_vec2 a, const YF_vec2 b);
YF_float yf_vec3_dot(const YF_vec3 a, const YF_vec3 b);
YF_float yf_vec4_dot(const YF_vec4 a, const YF_vec4 b);

/**
 * Computes the length of a vector.
 *
 * @param v: The vector.
 * @return: The length.
 */
YF_float yf_vec2_len(const YF_vec2 v);
YF_float yf_vec3_len(const YF_vec3 v);
YF_float yf_vec4_len(const YF_vec4 v);

/**
 * Normalizes a vector.
 *
 * @param dst: The destination vector.
 * @param v: The source vector.
 */
void yf_vec2_norm(YF_vec2 dst, const YF_vec2 v);
void yf_vec3_norm(YF_vec3 dst, const YF_vec3 v);
void yf_vec4_norm(YF_vec4 dst, const YF_vec4 v);

/**
 * Normalizes a vector in-place.
 *
 * @param v: The vector.
 */
void yf_vec2_normi(YF_vec2 v);
void yf_vec3_normi(YF_vec3 v);
void yf_vec4_normi(YF_vec4 v);

/**
 * Computes the cross product of two vectors.
 *
 * @param dst: The destination vector.
 * @param a: The first vector.
 * @param b: The second vector.
 */
void yf_vec3_cross(YF_vec3 dst, const YF_vec3 a, const YF_vec3 b);
void yf_vec4_cross(YF_vec4 dst, const YF_vec4 a, const YF_vec4 b);

/**
 * Computes a quaternion rotation for the x-, y-, or z-axis.
 *
 * The quaternion is encoded in a 'vec4' as follows:
 *  (r, v) : (vec4[3], vec4[0-2])
 *
 * @param q: The destination quaternion.
 * @param angle: The rotation angle, in radians.
 */
void yf_vec4_rotqx(YF_vec4 q, YF_float angle);
void yf_vec4_rotqy(YF_vec4 q, YF_float angle);
void yf_vec4_rotqz(YF_vec4 q, YF_float angle);

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
void yf_vec4_rotq(YF_vec4 q, YF_float angle, const YF_vec3 axis);

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
void yf_vec4_mulq(YF_vec4 dst, const YF_vec4 q1, const YF_vec4 q2);

/**
 * Multiplies a destination quaternion by another.
 *
 * The quaternion is encoded in a 'vec4' as follows:
 *  (r, v) : (vec4[3], vec4[0-2])
 *
 * @param dst: The destination quaternion.
 * @param q: The other quaternion.
 */
void yf_vec4_mulqi(YF_vec4 dst, const YF_vec4 q);

YF_DECLS_END

#endif /* YF_YF_VECTOR_H */
