/*
 * YF
 * matrix.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <string.h>
#include <math.h>

#include "matrix.h"

#define YF_MAT_SET(m, s, cn, rn) do { \
  for (unsigned i = 0; i < cn; ++i) { \
    for (unsigned j = 0; j < rn; ++j) \
      m[i*rn+j] = s; \
  } } while (0)

#define YF_MAT_XPOSE(dst, m, n) do { \
  for (unsigned i = 0; i < n; ++i) { \
    dst[i*(n+1)] = m[i*(n+1)]; \
    for (unsigned j = i + 1; j < n; ++j) { \
      dst[i*n+j] = m[j*n+i]; \
      dst[j*n+i] = m[i*n+j]; \
    } \
  } } while (0)

#define YF_MAT_SUB(dst, a, b, cn, rn) do { \
  for (unsigned i = 0; i < cn; ++i) { \
    for (unsigned j = 0; j < rn; ++j) \
      dst[i*rn+j] = a[i*rn+j] - b[i*rn+j]; \
  } } while (0)

#define YF_MAT_ADD(dst, a, b, cn, rn) do { \
  for (unsigned i = 0; i < cn; ++i) { \
    for (unsigned j = 0; j < rn; ++j) \
      dst[i*rn+j] = a[i*rn+j] + b[i*rn+j]; \
  } } while (0)

#define YF_MAT_MUL(dst, a, b, cn, rn, n) do { \
  for (unsigned i = 0; i < cn; ++i) { \
    for (unsigned j = 0; j < rn; ++j) { \
      dst[i*rn+j] = 0.0; \
      for (unsigned k = 0; k < n; ++k) \
        dst[i*rn+j] += a[k*n+j] * b[i*n+k]; \
    } \
  } } while (0)

#define YF_MAT_MULV(dst, m, v, n) do { \
  for (unsigned i = 0; i < n; ++i) { \
    dst[i] = 0.0; \
    for (unsigned j = 0; j < n; ++j) \
      dst[i] += m[j*n+i] * v[j]; \
  } } while (0)

void yf_mat2_iden(YF_mat2 m) {
  static const YF_mat2 iden = {
    1.0, 0.0,
    0.0, 1.0
  };
  memcpy(m, iden, sizeof iden);
}

void yf_mat3_iden(YF_mat3 m) {
  static const YF_mat3 iden = {
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0
  };
  memcpy(m, iden, sizeof iden);
}

void yf_mat4_iden(YF_mat4 m) {
  static const YF_mat4 iden = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
  };
  memcpy(m, iden, sizeof iden);
}

void yf_mat2_set(YF_mat2 m, YF_float s) {
  YF_MAT_SET(m, s, 2, 2);
}

void yf_mat3_set(YF_mat3 m, YF_float s) {
  YF_MAT_SET(m, s, 3, 3);
}

void yf_mat4_set(YF_mat4 m, YF_float s) {
  YF_MAT_SET(m, s, 4, 4);
}

void yf_mat2_copy(YF_mat2 dst, const YF_mat2 m) {
  memcpy(dst, m, sizeof(YF_mat2));
}

void yf_mat3_copy(YF_mat3 dst, const YF_mat3 m) {
  memcpy(dst, m, sizeof(YF_mat3));
}

void yf_mat4_copy(YF_mat4 dst, const YF_mat4 m) {
  memcpy(dst, m, sizeof(YF_mat4));
}

void yf_mat2_xpose(YF_mat2 dst, const YF_mat2 m) {
  YF_MAT_XPOSE(dst, m, 2);
}

void yf_mat3_xpose(YF_mat3 dst, const YF_mat3 m) {
  YF_MAT_XPOSE(dst, m, 3);
}

void yf_mat4_xpose(YF_mat4 dst, const YF_mat4 m) {
  YF_MAT_XPOSE(dst, m, 4);
}

void yf_mat2_sub(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b) {
  YF_MAT_SUB(dst, a, b, 2, 2);
}

void yf_mat3_sub(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b) {
  YF_MAT_SUB(dst, a, b, 3, 3);
}

void yf_mat4_sub(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b) {
  YF_MAT_SUB(dst, a, b, 4, 4);
}

void yf_mat2_add(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b) {
  YF_MAT_ADD(dst, a, b, 2, 2);
}

void yf_mat3_add(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b) {
  YF_MAT_ADD(dst, a, b, 3, 3);
}

void yf_mat4_add(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b) {
  YF_MAT_ADD(dst, a, b, 4, 4);
}

void yf_mat2_mul(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b) {
  YF_MAT_MUL(dst, a, b, 2, 2, 2);
}

void yf_mat3_mul(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b) {
  YF_MAT_MUL(dst, a, b, 3, 3, 3);
}

void yf_mat4_mul(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b) {
  YF_MAT_MUL(dst, a, b, 4, 4, 4);
}

void yf_mat2_mulv(YF_vec2 dst, const YF_mat2 m, const YF_vec2 v) {
  YF_MAT_MULV(dst, m, v, 2);
}

void yf_mat3_mulv(YF_vec3 dst, const YF_mat3 m, const YF_vec3 v) {
  YF_MAT_MULV(dst, m, v, 3);
}

void yf_mat4_mulv(YF_vec4 dst, const YF_mat4 m, const YF_vec4 v) {
  YF_MAT_MULV(dst, m, v, 4);
}

void yf_mat2_inv(YF_mat2 dst, const YF_mat2 m) {
  const YF_float idet = 1.0 / (m[0] * m[3] - m[1] * m[2]);
  dst[0] = +m[3] * idet;
  dst[1] = +m[1] * idet;
  dst[2] = -m[2] * idet;
  dst[3] = +m[0] * idet;
}

void yf_mat3_inv(YF_mat3 dst, const YF_mat3 m) {
  const YF_float s0 = m[4] * m[8] - m[5] * m[7];
  const YF_float s1 = m[3] * m[8] - m[5] * m[6];
  const YF_float s2 = m[3] * m[7] - m[4] * m[6];
  const YF_float idet = 1.0 / (m[0]*s0 - m[1]*s1 + m[2]*s2);
  dst[0] = +s0 * idet;
  dst[1] = -(m[1] * m[8] - m[2] * m[7]) * idet;
  dst[2] = +(m[1] * m[5] - m[2] * m[4]) * idet;
  dst[3] = -s1 * idet;
  dst[4] = +(m[0] * m[8] - m[2] * m[6]) * idet;
  dst[5] = -(m[0] * m[5] - m[2] * m[3]) * idet;
  dst[6] = +s2 * idet;
  dst[7] = -(m[0] * m[7] - m[1] * m[6]) * idet;
  dst[8] = +(m[0] * m[4] - m[1] * m[3]) * idet;
}

void yf_mat4_inv(YF_mat4 dst, const YF_mat4 m) {
  const YF_float s0 = m[0]  * m[5]  - m[1]  * m[4];
  const YF_float s1 = m[0]  * m[6]  - m[2]  * m[4];
  const YF_float s2 = m[0]  * m[7]  - m[3]  * m[4];
  const YF_float s3 = m[1]  * m[6]  - m[2]  * m[5];
  const YF_float s4 = m[1]  * m[7]  - m[3]  * m[5];
  const YF_float s5 = m[2]  * m[7]  - m[3]  * m[6];
  const YF_float c0 = m[8]  * m[13] - m[9]  * m[12];
  const YF_float c1 = m[8]  * m[14] - m[10] * m[12];
  const YF_float c2 = m[8]  * m[15] - m[11] * m[12];
  const YF_float c3 = m[9]  * m[14] - m[10] * m[13];
  const YF_float c4 = m[9]  * m[15] - m[11] * m[13];
  const YF_float c5 = m[10] * m[15] - m[11] * m[14];
  const YF_float idet = 1.0 / (s0*c5 - s1*c4 + s2*c3 + s3*c2 - s4*c1 + s5*c0);
  dst[0]  = (+c5 * m[5]  - c4 * m[6]  + c3 * m[7])  * idet;
  dst[1]  = (-c5 * m[1]  + c4 * m[2]  - c3 * m[3])  * idet;
  dst[2]  = (+s5 * m[13] - s4 * m[14] + s3 * m[15]) * idet;
  dst[3]  = (-s5 * m[9]  + s4 * m[10] - s3 * m[11]) * idet;
  dst[4]  = (-c5 * m[4]  + c2 * m[6]  - c1 * m[7])  * idet;
  dst[5]  = (+c5 * m[0]  - c2 * m[2]  + c1 * m[3])  * idet;
  dst[6]  = (-s5 * m[12] + s2 * m[14] - s1 * m[15]) * idet;
  dst[7]  = (+s5 * m[8]  - s2 * m[10] + s1 * m[11]) * idet;
  dst[8]  = (+c4 * m[4]  - c2 * m[5]  + c0 * m[7])  * idet;
  dst[9]  = (-c4 * m[0]  + c2 * m[1]  - c0 * m[3])  * idet;
  dst[10] = (+s4 * m[12] - s2 * m[13] + s0 * m[15]) * idet;
  dst[11] = (-s4 * m[8]  + s2 * m[9]  - s0 * m[11]) * idet;
  dst[12] = (-c3 * m[4]  + c1 * m[5]  - c0 * m[6])  * idet;
  dst[13] = (+c3 * m[0]  - c1 * m[1]  + c0 * m[2])  * idet;
  dst[14] = (-s3 * m[12] + s1 * m[13] - s0 * m[14]) * idet;
  dst[15] = (+s3 * m[8]  - s1 * m[9]  + s0 * m[10]) * idet;
}

void yf_mat3_rotx(YF_mat3 m, YF_float angle) {
#ifdef YF_USE_FLOAT64
  const YF_float c = cos(angle);
  const YF_float s = sin(angle);
#else
  const YF_float c = cosf(angle);
  const YF_float s = sinf(angle);
#endif
  yf_mat3_iden(m);
  m[4] = c;
  m[5] = s;
  m[7] = -s;
  m[8] = c;
}

void yf_mat3_roty(YF_mat3 m, YF_float angle) {
#ifdef YF_USE_FLOAT64
  const YF_float c = cos(angle);
  const YF_float s = sin(angle);
#else
  const YF_float c = cosf(angle);
  const YF_float s = sinf(angle);
#endif
  yf_mat3_iden(m);
  m[0] = c;
  m[2] = -s;
  m[6] = s;
  m[8] = c;
}

void yf_mat3_rotz(YF_mat3 m, YF_float angle) {
#ifdef YF_USE_FLOAT64
  const YF_float c = cos(angle);
  const YF_float s = sin(angle);
#else
  const YF_float c = cosf(angle);
  const YF_float s = sinf(angle);
#endif
  yf_mat3_iden(m);
  m[0] = c;
  m[1] = s;
  m[3] = -s;
  m[4] = c;
}

void yf_mat4_rotx(YF_mat4 m, YF_float angle) {
#ifdef YF_USE_FLOAT64
  const YF_float c = cos(angle);
  const YF_float s = sin(angle);
#else
  const YF_float c = cosf(angle);
  const YF_float s = sinf(angle);
#endif
  yf_mat4_iden(m);
  m[5] = c;
  m[6] = s;
  m[9] = -s;
  m[10] = c;
}

void yf_mat4_roty(YF_mat4 m, YF_float angle) {
#ifdef YF_USE_FLOAT64
  const YF_float c = cos(angle);
  const YF_float s = sin(angle);
#else
  const YF_float c = cosf(angle);
  const YF_float s = sinf(angle);
#endif
  yf_mat4_iden(m);
  m[0] = c;
  m[2] = -s;
  m[8] = s;
  m[10] = c;
}

void yf_mat4_rotz(YF_mat4 m, YF_float angle) {
#ifdef YF_USE_FLOAT64
  const YF_float c = cos(angle);
  const YF_float s = sin(angle);
#else
  const YF_float c = cosf(angle);
  const YF_float s = sinf(angle);
#endif
  yf_mat4_iden(m);
  m[0] = c;
  m[1] = s;
  m[4] = -s;
  m[5] = c;
}

void yf_mat3_rot(YF_mat3 m, YF_float angle, const YF_vec3 axis) {
  YF_vec3 v;
  yf_vec3_norm(v, axis);
  const YF_float x = v[0];
  const YF_float y = v[1];
  const YF_float z = v[2];
#ifdef YF_USE_FLOAT64
  const YF_float c = cos(angle);
  const YF_float s = sin(angle);
  const YF_float one = 1.0;
#else
  const YF_float c = cosf(angle);
  const YF_float s = sinf(angle);
  const YF_float one = 1.0f;
#endif
  m[0] = c + (one - c) * x*x;
  m[1] = (one - c) * x*y + s*z;
  m[2] = (one - c) * x*z - s*y;
  m[3] = (one - c) * x*y - s*z;
  m[4] = c + (one - c) * y*y;
  m[5] = (one - c) * y*z + s*x;
  m[6] = (one - c) * x*z + s*y;
  m[7] = (one - c) * y*z - s*x;
  m[8] = c + (one - c) * z*z;
}

void yf_mat4_rot(YF_mat4 m, YF_float angle, const YF_vec3 axis) {
  YF_vec3 v;
  yf_vec3_norm(v, axis);
  const YF_float x = v[0];
  const YF_float y = v[1];
  const YF_float z = v[2];
#ifdef YF_USE_FLOAT64
  const YF_float c = cos(angle);
  const YF_float s = sin(angle);
  const YF_float one = 1.0;
  const YF_float zero = 0.0;
#else
  const YF_float c = cosf(angle);
  const YF_float s = sinf(angle);
  const YF_float one = 1.0f;
  const YF_float zero = 0.0f;
#endif
  m[0] = c + (one - c) * x*x;
  m[1] = (one - c) * x*y + s*z;
  m[2] = (one - c) * x*z - s*y;
  m[3] = zero;
  m[4] = (one - c) * x*y - s*z;
  m[5] = c + (one - c) * y*y;
  m[6] = (one - c) * y*z + s*x;
  m[7] = zero;
  m[8] = (one - c) * x*z + s*y;
  m[9] = (one - c) * y*z - s*x;
  m[10] = c + (one - c) * z*z;
  m[11] = zero;
  m[12] = zero;
  m[13] = zero;
  m[14] = zero;
  m[15] = one;
}

void yf_mat3_rotq(YF_mat3 m, const YF_vec4 q) {
#ifdef YF_USE_FLOAT64
  const YF_float one = 1.0;
  const YF_float two = 2.0;
#else
  const YF_float one = 1.0f;
  const YF_float two = 2.0f;
#endif
  YF_vec4 u;
  yf_vec4_norm(u, q);
  const YF_float two_xw = two * u[0] * u[3];
  const YF_float two_xx = two * u[0] * u[0];
  const YF_float two_xy = two * u[0] * u[1];
  const YF_float two_xz = two * u[0] * u[2];
  const YF_float two_yw = two * u[1] * u[3];
  const YF_float two_yy = two * u[1] * u[1];
  const YF_float two_yz = two * u[1] * u[2];
  const YF_float two_zw = two * u[2] * u[3];
  const YF_float two_zz = two * u[2] * u[2];
  m[0] = one - two_yy - two_zz;
  m[1] = two_xy - two_zw;
  m[2] = two_xz + two_yw;
  m[3] = two_xy + two_zw;
  m[4] = one - two_xx - two_zz;
  m[5] = two_yz - two_xw;
  m[6] = two_xz - two_yw;
  m[7] = two_yz + two_xw;
  m[8] = one - two_xx - two_yy;
}

void yf_mat4_rotq(YF_mat4 m, const YF_vec4 q) {
#ifdef YF_USE_FLOAT64
  const YF_float one = 1.0;
  const YF_float two = 2.0;
  const YF_float zero = 0.0;
#else
  const YF_float one = 1.0f;
  const YF_float two = 2.0f;
  const YF_float zero = 0.0f;
#endif
  YF_vec4 u;
  yf_vec4_norm(u, q);
  const YF_float two_xw = two * u[0] * u[3];
  const YF_float two_xx = two * u[0] * u[0];
  const YF_float two_xy = two * u[0] * u[1];
  const YF_float two_xz = two * u[0] * u[2];
  const YF_float two_yw = two * u[1] * u[3];
  const YF_float two_yy = two * u[1] * u[1];
  const YF_float two_yz = two * u[1] * u[2];
  const YF_float two_zw = two * u[2] * u[3];
  const YF_float two_zz = two * u[2] * u[2];
  m[0] = one - two_yy - two_zz;
  m[1] = two_xy - two_zw;
  m[2] = two_xz + two_yw;
  m[3] = zero;
  m[4] = two_xy + two_zw;
  m[5] = one - two_xx - two_zz;
  m[6] = two_yz - two_xw;
  m[7] = zero;
  m[8] = two_xz - two_yw;
  m[9] = two_yz + two_xw;
  m[10] = one - two_xx - two_yy;
  m[11] = zero;
  m[12] = zero;
  m[13] = zero;
  m[14] = zero;
  m[15] = one;
}

void yf_mat3_scale(YF_mat3 m, YF_float sx, YF_float sy, YF_float sz) {
  memset(m, 0, sizeof(YF_mat3));
  m[0] = sx;
  m[4] = sy;
  m[8] = sz;
}

void yf_mat4_scale(YF_mat4 m, YF_float sx, YF_float sy, YF_float sz) {
  memset(m, 0, sizeof(YF_mat4));
  m[0] = sx;
  m[5] = sy;
  m[10] = sz;
  m[15] = 1.0;
}

void yf_mat4_xlate(YF_mat4 m, YF_float tx, YF_float ty, YF_float tz) {
  yf_mat4_iden(m);
  m[12] = tx;
  m[13] = ty;
  m[14] = tz;
}

void yf_mat4_lookat(YF_mat4 m, const YF_vec3 eye, const YF_vec3 center,
    const YF_vec3 up)
{
  YF_vec3 f, s, u;
  yf_vec3_sub(f, center, eye);
  yf_vec3_normi(f);
  yf_vec3_cross(s, f, up);
  yf_vec3_normi(s);
  yf_vec3_cross(u, f, s);
  m[0] = +s[0];
  m[1] = +u[0];
  m[2] = -f[0];
  m[3] = 0.0;
  m[4] = +s[1];
  m[5] = +u[1];
  m[6] = -f[1];
  m[7] = 0.0;
  m[8] = +s[2];
  m[9] = +u[2];
  m[10] = -f[2];
  m[11] = 0.0;
  m[12] = -yf_vec3_dot(s, eye);
  m[13] = -yf_vec3_dot(u, eye);
  m[14] = +yf_vec3_dot(f, eye);
  m[15] = 1.0;
}

void yf_mat4_persp(YF_mat4 m, YF_float yfov, YF_float aspect,
    YF_float znear, YF_float zfar)
{
#ifdef YF_USE_FLOAT64
  const YF_float one = 1.0;
  const YF_float ct = one / tan(yfov * 0.5);
#else
  const YF_float one = 1.0f;
  const YF_float ct = one / tanf(yfov * 0.5f);
#endif
  memset(m, 0, sizeof(YF_mat4));
  m[0] = ct / aspect;
  m[5] = ct;
  m[10] = -(zfar + znear) / (zfar - znear);
  m[11] = -one;
  m[14] = -((one+one) * zfar * znear) / (zfar - znear);
}

void yf_mat4_ortho(YF_mat4 m, YF_float left, YF_float right,
    YF_float top, YF_float bottom, YF_float znear, YF_float zfar)
{
#ifdef YF_USE_FLOAT64
  const YF_float one = 1.0;
  const YF_float two = 2.0;
#else
  const YF_float one = 1.0f;
  const YF_float two = 2.0f;
#endif
  memset(m, 0, sizeof(YF_mat4));
  m[0] = +two / (right - left);
  m[5] = +two / (bottom - top);
  m[10] = -two / (zfar - znear);
  m[12] = -(right + left) / (right - left);
  m[13] = -(bottom + top) / (bottom - top);
  m[14] = -(zfar + znear) / (zfar - znear);
  m[15] = one;
}
