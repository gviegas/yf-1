/*
 * YF
 * vector.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <string.h>
#include <math.h>

#include "vector.h"

#ifdef YF_USE_FLOAT64
# define YF_FLT_ISEQ(a, b) (fabs(a-b) < 1e-15)
#else
# define YF_FLT_ISEQ(a, b) (fabsf(a-b) < 1e-6)
#endif

#define YF_VEC_ISEQ(r, a, b, n) do { \
  for (unsigned i = 0; i < n; ++i) { \
    if (!(r = YF_FLT_ISEQ(a[i], b[i]))) \
      break; \
  } } while (0);

#define YF_VEC_SET(v, s, n) do { \
  for (unsigned i = 0; i < n; ++i) \
    v[i] = s; } while (0)

#define YF_VEC_SUB(dst, a, b, n) do { \
  for (unsigned i = 0; i < n; ++i) \
    dst[i] = a[i] - b[i]; } while (0)

#define YF_VEC_ADD(dst, a, b, n) do { \
  for (unsigned i = 0; i < n; ++i) \
    dst[i] = a[i] + b[i]; } while (0)

#define YF_VEC_MULS(dst, v, s, n) do { \
  for (unsigned i = 0; i < n; ++i) \
    dst[i] = v[i] * s; } while (0)

#define YF_VEC_DOT(r, a, b, n) do { \
  r = 0.0; \
  for (unsigned i = 0; i < n; ++i) \
    r += a[i] * b[i]; } while (0)

int yf_vec2_iszero(const YF_vec2 v) {
  static const YF_vec2 zero = {0.0, 0.0};
  int r;
  YF_VEC_ISEQ(r, v, zero, 2);
  return r;
}

int yf_vec3_iszero(const YF_vec3 v) {
  static const YF_vec3 zero = {0.0, 0.0, 0.0};
  int r;
  YF_VEC_ISEQ(r, v, zero, 3);
  return r;
}

int yf_vec4_iszero(const YF_vec4 v) {
  static const YF_vec4 zero = {0.0, 0.0, 0.0, 0.0};
  int r;
  YF_VEC_ISEQ(r, v, zero, 4);
  return r;
}

int yf_vec2_iseq(const YF_vec2 a, const YF_vec2 b) {
  int r;
  YF_VEC_ISEQ(r, a, b, 2);
  return r;
}

int yf_vec3_iseq(const YF_vec3 a, const YF_vec3 b) {
  int r;
  YF_VEC_ISEQ(r, a, b, 3);
  return r;
}

int yf_vec4_iseq(const YF_vec4 a, const YF_vec4 b) {
  int r;
  YF_VEC_ISEQ(r, a, b, 4);
  return r;
}

void yf_vec2_set(YF_vec2 v, YF_float s) {
  YF_VEC_SET(v, s, 2);
}

void yf_vec3_set(YF_vec3 v, YF_float s) {
  YF_VEC_SET(v, s, 3);
}

void yf_vec4_set(YF_vec4 v, YF_float s) {
  YF_VEC_SET(v, s, 4);
}

void yf_vec2_copy(YF_vec2 dst, const YF_vec2 v) {
  memcpy(dst, v, sizeof(YF_vec2));
}

void yf_vec3_copy(YF_vec3 dst, const YF_vec3 v) {
  memcpy(dst, v, sizeof(YF_vec3));
}

void yf_vec4_copy(YF_vec4 dst, const YF_vec4 v) {
  memcpy(dst, v, sizeof(YF_vec4));
}

void yf_vec2_sub(YF_vec2 dst, const YF_vec2 a, const YF_vec2 b) {
  YF_VEC_SUB(dst, a, b, 2);
}

void yf_vec3_sub(YF_vec3 dst, const YF_vec3 a, const YF_vec3 b) {
  YF_VEC_SUB(dst, a, b, 3);
}

void yf_vec4_sub(YF_vec4 dst, const YF_vec4 a, const YF_vec4 b) {
  YF_VEC_SUB(dst, a, b, 4);
}

void yf_vec2_subi(YF_vec2 dst, const YF_vec2 v) {
  YF_VEC_SUB(dst, dst, v, 2);
}

void yf_vec3_subi(YF_vec3 dst, const YF_vec3 v) {
  YF_VEC_SUB(dst, dst, v, 3);
}

void yf_vec4_subi(YF_vec4 dst, const YF_vec4 v) {
  YF_VEC_SUB(dst, dst, v, 4);
}

void yf_vec2_add(YF_vec2 dst, const YF_vec2 a, const YF_vec2 b) {
  YF_VEC_ADD(dst, a, b, 2);
}

void yf_vec3_add(YF_vec3 dst, const YF_vec3 a, const YF_vec3 b) {
  YF_VEC_ADD(dst, a, b, 3);
}

void yf_vec4_add(YF_vec4 dst, const YF_vec4 a, const YF_vec4 b) {
  YF_VEC_ADD(dst, a, b, 4);
}

void yf_vec2_addi(YF_vec2 dst, const YF_vec2 v) {
  YF_VEC_ADD(dst, dst, v, 2);
}

void yf_vec3_addi(YF_vec3 dst, const YF_vec3 v) {
  YF_VEC_ADD(dst, dst, v, 3);
}

void yf_vec4_addi(YF_vec4 dst, const YF_vec4 v) {
  YF_VEC_ADD(dst, dst, v, 4);
}

void yf_vec2_muls(YF_vec2 dst, const YF_vec2 v, YF_float s) {
  YF_VEC_MULS(dst, v, s, 2);
}

void yf_vec3_muls(YF_vec3 dst, const YF_vec3 v, YF_float s) {
  YF_VEC_MULS(dst, v, s, 3);
}

void yf_vec4_muls(YF_vec4 dst, const YF_vec4 v, YF_float s) {
  YF_VEC_MULS(dst, v, s, 4);
}

void yf_vec2_mulsi(YF_vec2 dst, YF_float s) {
  YF_VEC_MULS(dst, dst, s, 2);
}

void yf_vec3_mulsi(YF_vec3 dst, YF_float s) {
  YF_VEC_MULS(dst, dst, s, 3);
}

void yf_vec4_mulsi(YF_vec4 dst, YF_float s) {
  YF_VEC_MULS(dst, dst, s, 4);
}

YF_float yf_vec2_dot(const YF_vec2 a, const YF_vec2 b) {
  YF_float r;
  YF_VEC_DOT(r, a, b, 2);
  return r;
}

YF_float yf_vec3_dot(const YF_vec3 a, const YF_vec3 b) {
  YF_float r;
  YF_VEC_DOT(r, a, b, 3);
  return r;
}

YF_float yf_vec4_dot(const YF_vec4 a, const YF_vec4 b) {
  YF_float r;
  YF_VEC_DOT(r, a, b, 4);
  return r;
}

YF_float yf_vec2_len(const YF_vec2 v) {
#ifdef YF_USE_FLOAT64
  return sqrt(yf_vec2_dot(v, v));
#else
  return sqrtf(yf_vec2_dot(v, v));
#endif
}

YF_float yf_vec3_len(const YF_vec3 v) {
#ifdef YF_USE_FLOAT64
  return sqrt(yf_vec3_dot(v, v));
#else
  return sqrtf(yf_vec3_dot(v, v));
#endif
}

YF_float yf_vec4_len(const YF_vec4 v) {
#ifdef YF_USE_FLOAT64
  return sqrt(yf_vec4_dot(v, v));
#else
  return sqrtf(yf_vec4_dot(v, v));
#endif
}

void yf_vec2_norm(YF_vec2 dst, const YF_vec2 v) {
  const YF_float s = 1.0 / yf_vec2_len(v);
  YF_VEC_MULS(dst, v, s, 2);
}

void yf_vec3_norm(YF_vec3 dst, const YF_vec3 v) {
  const YF_float s = 1.0 / yf_vec3_len(v);
  YF_VEC_MULS(dst, v, s, 3);
}

void yf_vec4_norm(YF_vec4 dst, const YF_vec4 v) {
  const YF_float s = 1.0 / yf_vec4_len(v);
  YF_VEC_MULS(dst, v, s, 4);
}

void yf_vec2_normi(YF_vec2 v) {
  yf_vec2_norm(v, v);
}

void yf_vec3_normi(YF_vec3 v) {
  yf_vec3_norm(v, v);
}

void yf_vec4_normi(YF_vec4 v) {
  yf_vec4_norm(v, v);
}

void yf_vec3_cross(YF_vec3 dst, const YF_vec3 a, const YF_vec3 b) {
  dst[0] = a[1] * b[2] - a[2] * b[1];
  dst[1] = a[2] * b[0] - a[0] * b[2];
  dst[2] = a[0] * b[1] - a[1] * b[0];
}

void yf_vec4_cross(YF_vec4 dst, const YF_vec4 a, const YF_vec4 b) {
  yf_vec3_cross(dst, a, b);
  dst[3] = 1.0;
}

void yf_vec4_rotq(YF_vec4 q, YF_float angle, const YF_vec3 axis) {
  YF_vec3 v;
  yf_vec3_norm(v, axis);
#ifdef YF_USE_FLOAT64
  const YF_float a = angle * 0.5;
  const YF_float c = cos(a);
  const YF_float s = sin(a);
#else
  const YF_float a = angle * 0.5f;
  const YF_float c = cosf(a);
  const YF_float s = sinf(a);
#endif
  q[3] = c;
  q[0] = s * v[0];
  q[1] = s * v[1];
  q[2] = s * v[2];
}

void yf_vec4_rotqx(YF_vec4 q, YF_float angle) {
#ifdef YF_USE_FLOAT64
  const YF_float a = angle * 0.5;
  q[3] = cos(a);
  q[0] = sin(a);
  q[1] = q[2] = 0.0;
#else
  const YF_float a = angle * 0.5f;
  q[3] = cosf(a);
  q[0] = sinf(a);
  q[1] = q[2] = 0.0f;
#endif
}

void yf_vec4_rotqy(YF_vec4 q, YF_float angle) {
#ifdef YF_USE_FLOAT64
  const YF_float a = angle * 0.5;
  q[3] = cos(a);
  q[1] = sin(a);
  q[0] = q[2] = 0.0;
#else
  const YF_float a = angle * 0.5f;
  q[3] = cosf(a);
  q[1] = sinf(a);
  q[0] = q[2] = 0.0f;
#endif
}

void yf_vec4_rotqz(YF_vec4 q, YF_float angle) {
#ifdef YF_USE_FLOAT64
  const YF_float a = angle * 0.5;
  q[3] = cos(a);
  q[2] = sin(a);
  q[0] = q[1] = 0.0;
#else
  const YF_float a = angle * 0.5f;
  q[3] = cosf(a);
  q[2] = sinf(a);
  q[0] = q[1] = 0.0f;
#endif
}

void yf_vec4_mulq(YF_vec4 dst, const YF_vec4 q1, const YF_vec4 q2) {
  YF_vec3 v, u;
  yf_vec3_muls(v, q2, q1[3]);
  yf_vec3_muls(u, q1, q2[3]);
  yf_vec3_addi(v, u);
  yf_vec3_cross(u, q1, q2);
  yf_vec3_add(dst, v, u);
  dst[3] = q1[3] * q2[3] - yf_vec3_dot(q1, q2);
}

void yf_vec4_mulqi(YF_vec4 dst, const YF_vec4 q) {
  const YF_float r = dst[3] * q[3] - yf_vec3_dot(dst, q);
  YF_vec3 v, u;
  yf_vec3_cross(v, dst, q);
  yf_vec3_mulsi(dst, q[3]);
  yf_vec3_muls(u, q, dst[3]);
  yf_vec3_addi(dst, u);
  yf_vec3_addi(dst, v);
  dst[3] = r;
}
