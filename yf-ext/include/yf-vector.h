/*
 * YF
 * yf-vector.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_VECTOR_H
#define YF_YF_VECTOR_H

#include <yf/com/yf-defs.h>
#include <yf/com/yf-types.h>

YF_DECLS_BEGIN

/* 2-, 3- and 4-component vectors. */
typedef YF_float YF_vec2[2];
typedef YF_float YF_vec3[3];
typedef YF_float YF_vec4[4];

/* Checks whether or not a given vector is the zero vector. */
int yf_vec2_iszero(const YF_vec2 v);
int yf_vec3_iszero(const YF_vec3 v);
int yf_vec4_iszero(const YF_vec4 v);

/* Checks whether or not a given vector is equal to another. */
int yf_vec2_iseq(const YF_vec2 a, const YF_vec2 b);
int yf_vec3_iseq(const YF_vec3 a, const YF_vec3 b);
int yf_vec4_iseq(const YF_vec4 a, const YF_vec4 b);

/* Sets all vector components to a given scalar. */
void yf_vec2_set(YF_vec2 v, YF_float s);
void yf_vec3_set(YF_vec3 v, YF_float s);
void yf_vec4_set(YF_vec4 v, YF_float s);

/* Copies one vector to another. */
void yf_vec2_copy(YF_vec2 dst, const YF_vec2 v);
void yf_vec3_copy(YF_vec3 dst, const YF_vec3 v);
void yf_vec4_copy(YF_vec4 dst, const YF_vec4 v);

/* Subtracts two vectors. */
void yf_vec2_sub(YF_vec2 dst, const YF_vec2 a, const YF_vec2 b);
void yf_vec3_sub(YF_vec3 dst, const YF_vec3 a, const YF_vec3 b);
void yf_vec4_sub(YF_vec4 dst, const YF_vec4 a, const YF_vec4 b);

/* Subtracts a given vector from a destination vector. */
void yf_vec2_subi(YF_vec2 dst, const YF_vec2 v);
void yf_vec3_subi(YF_vec3 dst, const YF_vec3 v);
void yf_vec4_subi(YF_vec4 dst, const YF_vec4 v);

/* Adds two vectors. */
void yf_vec2_add(YF_vec2 dst, const YF_vec2 a, const YF_vec2 b);
void yf_vec3_add(YF_vec3 dst, const YF_vec3 a, const YF_vec3 b);
void yf_vec4_add(YF_vec4 dst, const YF_vec4 a, const YF_vec4 b);

/* Adds a given vector to a destination vector. */
void yf_vec2_addi(YF_vec2 dst, const YF_vec2 v);
void yf_vec3_addi(YF_vec3 dst, const YF_vec3 v);
void yf_vec4_addi(YF_vec4 dst, const YF_vec4 v);

/* Multiplies a vector and a scalar. */
void yf_vec2_muls(YF_vec2 dst, const YF_vec2 v, YF_float s);
void yf_vec3_muls(YF_vec3 dst, const YF_vec3 v, YF_float s);
void yf_vec4_muls(YF_vec4 dst, const YF_vec4 v, YF_float s);

/* Multiplies a destination vector by a given scalar. */
void yf_vec2_mulsi(YF_vec2 dst, YF_float s);
void yf_vec3_mulsi(YF_vec3 dst, YF_float s);
void yf_vec4_mulsi(YF_vec4 dst, YF_float s);

/* Computes the dot product of two vectors. */
YF_float yf_vec2_dot(const YF_vec2 a, const YF_vec2 b);
YF_float yf_vec3_dot(const YF_vec3 a, const YF_vec3 b);
YF_float yf_vec4_dot(const YF_vec4 a, const YF_vec4 b);

/* Computes the length of a vector. */
YF_float yf_vec2_len(const YF_vec2 v);
YF_float yf_vec3_len(const YF_vec3 v);
YF_float yf_vec4_len(const YF_vec4 v);

/* Normalizes a vector. */
void yf_vec2_norm(YF_vec2 dst, const YF_vec2 v);
void yf_vec3_norm(YF_vec3 dst, const YF_vec3 v);
void yf_vec4_norm(YF_vec4 dst, const YF_vec4 v);

/* Normalizes a vector in-place. */
void yf_vec2_normi(YF_vec2 v);
void yf_vec3_normi(YF_vec3 v);
void yf_vec4_normi(YF_vec4 v);

/* Computes the cross product of two vectors. */
void yf_vec3_cross(YF_vec3 dst, const YF_vec3 a, const YF_vec3 b);
void yf_vec4_cross(YF_vec4 dst, const YF_vec4 a, const YF_vec4 b);

YF_DECLS_END

#endif /* YF_YF_VECTOR_H */
