/*
 * YF
 * matrix.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <string.h>
#include <math.h>

#include "yf-matrix.h"

#define YF_MAT_SET(m, s, cn, rn) do { \
    for (unsigned i = 0; i < (cn); i++) { \
        for (unsigned j = 0; j < (rn); j++) \
            (m)[i*(rn)+j] = s; \
    } } while (0)

#define YF_MAT_XPOSE(dst, m, n) do { \
    for (unsigned i = 0; i < (n); i++) { \
        (dst)[i*((n)+1)] = (m)[i*((n)+1)]; \
        for (unsigned j = i + 1; j < (n); j++) { \
            (dst)[i*(n)+j] = (m)[j*(n)+i]; \
            (dst)[j*(n)+i] = (m)[i*(n)+j]; \
        } \
    } } while (0)

#define YF_MAT_SUB(dst, a, b, cn, rn) do { \
    for (unsigned i = 0; i < (cn); i++) { \
        for (unsigned j = 0; j < (rn); j++) \
            (dst)[i*(rn)+j] = (a)[i*(rn)+j] - (b)[i*(rn)+j]; \
    } } while (0)

#define YF_MAT_ADD(dst, a, b, cn, rn) do { \
    for (unsigned i = 0; i < (cn); i++) { \
        for (unsigned j = 0; j < (rn); j++) \
            (dst)[i*(rn)+j] = (a)[i*(rn)+j] + (b)[i*(rn)+j]; \
    } } while (0)

#define YF_MAT_MUL(dst, a, b, cn, rn, n) do { \
    for (unsigned i = 0; i < (cn); i++) { \
        for (unsigned j = 0; j < (rn); j++) { \
            (dst)[i*(rn)+j] = 0.0f; \
            for (unsigned k = 0; k < (n); k++) \
                (dst)[i*(rn)+j] += (a)[k*(n)+j] * (b)[i*(n)+k]; \
        } \
    } } while (0)

#define YF_MAT_MULV(dst, m, v, n) do { \
    for (unsigned i = 0; i < (n); i++) { \
        (dst)[i] = 0.0f; \
        for (unsigned j = 0; j < (n); j++) \
            (dst)[i] += (m)[j*(n)+i] * (v)[j]; \
    } } while (0)

void yf_mat2_iden(YF_mat2 m)
{
    static const YF_mat2 iden = {
        1.0f, 0.0f,
        0.0f, 1.0f
    };
    memcpy(m, iden, sizeof iden);
}

void yf_mat3_iden(YF_mat3 m)
{
    static const YF_mat3 iden = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
    memcpy(m, iden, sizeof iden);
}

void yf_mat4_iden(YF_mat4 m)
{
    static const YF_mat4 iden = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    memcpy(m, iden, sizeof iden);
}

void yf_mat2_set(YF_mat2 m, float s)
{
    YF_MAT_SET(m, s, 2, 2);
}

void yf_mat3_set(YF_mat3 m, float s)
{
    YF_MAT_SET(m, s, 3, 3);
}

void yf_mat4_set(YF_mat4 m, float s)
{
    YF_MAT_SET(m, s, 4, 4);
}

void yf_mat2_copy(YF_mat2 dst, const YF_mat2 m)
{
    memcpy(dst, m, sizeof(YF_mat2));
}

void yf_mat3_copy(YF_mat3 dst, const YF_mat3 m)
{
    memcpy(dst, m, sizeof(YF_mat3));
}

void yf_mat4_copy(YF_mat4 dst, const YF_mat4 m)
{
    memcpy(dst, m, sizeof(YF_mat4));
}

void yf_mat2_xpose(YF_mat2 dst, const YF_mat2 m)
{
    YF_MAT_XPOSE(dst, m, 2);
}

void yf_mat3_xpose(YF_mat3 dst, const YF_mat3 m)
{
    YF_MAT_XPOSE(dst, m, 3);
}

void yf_mat4_xpose(YF_mat4 dst, const YF_mat4 m)
{
    YF_MAT_XPOSE(dst, m, 4);
}

void yf_mat2_sub(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b)
{
    YF_MAT_SUB(dst, a, b, 2, 2);
}

void yf_mat3_sub(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b)
{
    YF_MAT_SUB(dst, a, b, 3, 3);
}

void yf_mat4_sub(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b)
{
    YF_MAT_SUB(dst, a, b, 4, 4);
}

void yf_mat2_add(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b)
{
    YF_MAT_ADD(dst, a, b, 2, 2);
}

void yf_mat3_add(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b)
{
    YF_MAT_ADD(dst, a, b, 3, 3);
}

void yf_mat4_add(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b)
{
    YF_MAT_ADD(dst, a, b, 4, 4);
}

void yf_mat2_mul(YF_mat2 dst, const YF_mat2 a, const YF_mat2 b)
{
    YF_MAT_MUL(dst, a, b, 2, 2, 2);
}

void yf_mat3_mul(YF_mat3 dst, const YF_mat3 a, const YF_mat3 b)
{
    YF_MAT_MUL(dst, a, b, 3, 3, 3);
}

void yf_mat4_mul(YF_mat4 dst, const YF_mat4 a, const YF_mat4 b)
{
    YF_MAT_MUL(dst, a, b, 4, 4, 4);
}

void yf_mat2_mulv(YF_vec2 dst, const YF_mat2 m, const YF_vec2 v)
{
    YF_MAT_MULV(dst, m, v, 2);
}

void yf_mat3_mulv(YF_vec3 dst, const YF_mat3 m, const YF_vec3 v)
{
    YF_MAT_MULV(dst, m, v, 3);
}

void yf_mat4_mulv(YF_vec4 dst, const YF_mat4 m, const YF_vec4 v)
{
    YF_MAT_MULV(dst, m, v, 4);
}

void yf_mat2_inv(YF_mat2 dst, const YF_mat2 m)
{
    const float idet = 1.0f / (m[0] * m[3] - m[1] * m[2]);
    dst[0] = +m[3] * idet;
    dst[1] = +m[1] * idet;
    dst[2] = -m[2] * idet;
    dst[3] = +m[0] * idet;
}

void yf_mat3_inv(YF_mat3 dst, const YF_mat3 m)
{
    const float s0 = m[4] * m[8] - m[5] * m[7];
    const float s1 = m[3] * m[8] - m[5] * m[6];
    const float s2 = m[3] * m[7] - m[4] * m[6];
    const float idet = 1.0f / (m[0]*s0 - m[1]*s1 + m[2]*s2);
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

void yf_mat4_inv(YF_mat4 dst, const YF_mat4 m)
{
    const float s0 = m[0]  * m[5]  - m[1]  * m[4];
    const float s1 = m[0]  * m[6]  - m[2]  * m[4];
    const float s2 = m[0]  * m[7]  - m[3]  * m[4];
    const float s3 = m[1]  * m[6]  - m[2]  * m[5];
    const float s4 = m[1]  * m[7]  - m[3]  * m[5];
    const float s5 = m[2]  * m[7]  - m[3]  * m[6];
    const float c0 = m[8]  * m[13] - m[9]  * m[12];
    const float c1 = m[8]  * m[14] - m[10] * m[12];
    const float c2 = m[8]  * m[15] - m[11] * m[12];
    const float c3 = m[9]  * m[14] - m[10] * m[13];
    const float c4 = m[9]  * m[15] - m[11] * m[13];
    const float c5 = m[10] * m[15] - m[11] * m[14];
    const float idet = 1.0f / (s0*c5 - s1*c4 + s2*c3 + s3*c2 - s4*c1 + s5*c0);
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

void yf_mat3_rotx(YF_mat3 m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat3_iden(m);
    m[4] = c;
    m[5] = s;
    m[7] = -s;
    m[8] = c;
}

void yf_mat3_roty(YF_mat3 m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat3_iden(m);
    m[0] = c;
    m[2] = -s;
    m[6] = s;
    m[8] = c;
}

void yf_mat3_rotz(YF_mat3 m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat3_iden(m);
    m[0] = c;
    m[1] = s;
    m[3] = -s;
    m[4] = c;
}

void yf_mat4_rotx(YF_mat4 m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat4_iden(m);
    m[5] = c;
    m[6] = s;
    m[9] = -s;
    m[10] = c;
}

void yf_mat4_roty(YF_mat4 m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat4_iden(m);
    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;
}

void yf_mat4_rotz(YF_mat4 m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat4_iden(m);
    m[0] = c;
    m[1] = s;
    m[4] = -s;
    m[5] = c;
}

void yf_mat3_rot(YF_mat3 m, float angle, const YF_vec3 axis)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    const float one_minus_c = 1.0f - c;
    YF_vec3 v;
    yf_vec3_norm(v, axis);
    const float xx = v[0] * v[0];
    const float xy = v[0] * v[1];
    const float xz = v[0] * v[2];
    const float yy = v[1] * v[1];
    const float yz = v[1] * v[2];
    const float zz = v[2] * v[2];
    const float sx = s * v[0];
    const float sy = s * v[1];
    const float sz = s * v[2];
    m[0] = c + one_minus_c * xx;
    m[1] = one_minus_c * xy + sz;
    m[2] = one_minus_c * xz - sy;
    m[3] = one_minus_c * xy - sz;
    m[4] = c + one_minus_c * yy;
    m[5] = one_minus_c * yz + sx;
    m[6] = one_minus_c * xz + sy;
    m[7] = one_minus_c * yz - sx;
    m[8] = c + one_minus_c * zz;
}

void yf_mat4_rot(YF_mat4 m, float angle, const YF_vec3 axis)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    const float one_minus_c = 1.0f - c;
    YF_vec3 v;
    yf_vec3_norm(v, axis);
    const float xx = v[0] * v[0];
    const float xy = v[0] * v[1];
    const float xz = v[0] * v[2];
    const float yy = v[1] * v[1];
    const float yz = v[1] * v[2];
    const float zz = v[2] * v[2];
    const float sx = s * v[0];
    const float sy = s * v[1];
    const float sz = s * v[2];
    yf_mat4_iden(m);
    m[0] = c + one_minus_c * xx;
    m[1] = one_minus_c * xy + sz;
    m[2] = one_minus_c * xz - sy;
    m[4] = one_minus_c * xy - sz;
    m[5] = c + one_minus_c * yy;
    m[6] = one_minus_c * yz + sx;
    m[8] = one_minus_c * xz + sy;
    m[9] = one_minus_c * yz - sx;
    m[10] = c + one_minus_c * zz;
}

void yf_mat3_rotq(YF_mat3 m, const YF_vec4 q)
{
    YF_vec4 u;
    yf_vec4_norm(u, q);
    const float two_xw = 2.0f * u[0] * u[3];
    const float two_xx = 2.0f * u[0] * u[0];
    const float two_xy = 2.0f * u[0] * u[1];
    const float two_xz = 2.0f * u[0] * u[2];
    const float two_yw = 2.0f * u[1] * u[3];
    const float two_yy = 2.0f * u[1] * u[1];
    const float two_yz = 2.0f * u[1] * u[2];
    const float two_zw = 2.0f * u[2] * u[3];
    const float two_zz = 2.0f * u[2] * u[2];
    m[0] = 1.0f - two_yy - two_zz;
    m[1] = two_xy + two_zw;
    m[2] = two_xz - two_yw;
    m[3] = two_xy - two_zw;
    m[4] = 1.0f - two_xx - two_zz;
    m[5] = two_yz + two_xw;
    m[6] = two_xz + two_yw;
    m[7] = two_yz - two_xw;
    m[8] = 1.0f - two_xx - two_yy;
}

void yf_mat4_rotq(YF_mat4 m, const YF_vec4 q)
{
    YF_vec4 u;
    yf_vec4_norm(u, q);
    const float two_xw = 2.0f * u[0] * u[3];
    const float two_xx = 2.0f * u[0] * u[0];
    const float two_xy = 2.0f * u[0] * u[1];
    const float two_xz = 2.0f * u[0] * u[2];
    const float two_yw = 2.0f * u[1] * u[3];
    const float two_yy = 2.0f * u[1] * u[1];
    const float two_yz = 2.0f * u[1] * u[2];
    const float two_zw = 2.0f * u[2] * u[3];
    const float two_zz = 2.0f * u[2] * u[2];
    yf_mat4_iden(m);
    m[0] = 1.0f - two_yy - two_zz;
    m[1] = two_xy + two_zw;
    m[2] = two_xz - two_yw;
    m[4] = two_xy - two_zw;
    m[5] = 1.0f - two_xx - two_zz;
    m[6] = two_yz + two_xw;
    m[8] = two_xz + two_yw;
    m[9] = two_yz - two_xw;
    m[10] = 1.0f - two_xx - two_yy;
}

void yf_mat3_scale(YF_mat3 m, float sx, float sy, float sz)
{
    memset(m, 0, sizeof(YF_mat3));
    m[0] = sx;
    m[4] = sy;
    m[8] = sz;
}

void yf_mat4_scale(YF_mat4 m, float sx, float sy, float sz)
{
    memset(m, 0, sizeof(YF_mat4));
    m[0] = sx;
    m[5] = sy;
    m[10] = sz;
    m[15] = 1.0f;
}

void yf_mat4_xlate(YF_mat4 m, float tx, float ty, float tz)
{
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
    m[3] = 0.0f;
    m[4] = +s[1];
    m[5] = +u[1];
    m[6] = -f[1];
    m[7] = 0.0f;
    m[8] = +s[2];
    m[9] = +u[2];
    m[10] = -f[2];
    m[11] = 0.0f;
    m[12] = -yf_vec3_dot(s, eye);
    m[13] = -yf_vec3_dot(u, eye);
    m[14] = +yf_vec3_dot(f, eye);
    m[15] = 1.0f;
}

void yf_mat4_persp(YF_mat4 m, float yfov, float aspect, float znear,
                   float zfar)
{
    const float ct = 1.0f / tanf(yfov * 0.5f);
    memset(m, 0, sizeof(YF_mat4));
    m[0] = ct / aspect;
    m[5] = ct;
    m[10] = (zfar + znear) / (znear - zfar);
    m[11] = -1.0f;
    m[14] = (2.0f * zfar * znear) / (znear - zfar);
}

void yf_mat4_infpersp(YF_mat4 m, float yfov, float aspect, float znear)
{
    const float ct = 1.0f / tanf(yfov * 0.5f);
    memset(m, 0, sizeof(YF_mat4));
    m[0] = ct / aspect;
    m[5] = ct;
    m[10] = m[11] = -1.0f;
    m[14] = -2.0f * znear;
}

void yf_mat4_ortho(YF_mat4 m, float xmag, float ymag, float znear, float zfar)
{
    memset(m, 0, sizeof(YF_mat4));
    m[0] = 1.0f / xmag;
    m[5] = 1.0f / ymag;
    m[10] = 2.0f / (znear - zfar);
    m[14] = (zfar + znear) / (znear - zfar);
    m[15] = 1.0f;
}
