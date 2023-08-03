/*
 * YF
 * matrix.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
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

void yf_mat2_iden(yf_mat2_t m)
{
    static const yf_mat2_t iden = {
        1.0f, 0.0f,
        0.0f, 1.0f
    };
    memcpy(m, iden, sizeof iden);
}

void yf_mat3_iden(yf_mat3_t m)
{
    static const yf_mat3_t iden = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
    memcpy(m, iden, sizeof iden);
}

void yf_mat4_iden(yf_mat4_t m)
{
    static const yf_mat4_t iden = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    memcpy(m, iden, sizeof iden);
}

void yf_mat2_set(yf_mat2_t m, float s)
{
    YF_MAT_SET(m, s, 2, 2);
}

void yf_mat3_set(yf_mat3_t m, float s)
{
    YF_MAT_SET(m, s, 3, 3);
}

void yf_mat4_set(yf_mat4_t m, float s)
{
    YF_MAT_SET(m, s, 4, 4);
}

void yf_mat2_copy(yf_mat2_t dst, const yf_mat2_t m)
{
    memcpy(dst, m, sizeof(yf_mat2_t));
}

void yf_mat3_copy(yf_mat3_t dst, const yf_mat3_t m)
{
    memcpy(dst, m, sizeof(yf_mat3_t));
}

void yf_mat4_copy(yf_mat4_t dst, const yf_mat4_t m)
{
    memcpy(dst, m, sizeof(yf_mat4_t));
}

void yf_mat2_xpose(yf_mat2_t dst, const yf_mat2_t m)
{
    YF_MAT_XPOSE(dst, m, 2);
}

void yf_mat3_xpose(yf_mat3_t dst, const yf_mat3_t m)
{
    YF_MAT_XPOSE(dst, m, 3);
}

void yf_mat4_xpose(yf_mat4_t dst, const yf_mat4_t m)
{
    YF_MAT_XPOSE(dst, m, 4);
}

void yf_mat2_sub(yf_mat2_t dst, const yf_mat2_t a, const yf_mat2_t b)
{
    YF_MAT_SUB(dst, a, b, 2, 2);
}

void yf_mat3_sub(yf_mat3_t dst, const yf_mat3_t a, const yf_mat3_t b)
{
    YF_MAT_SUB(dst, a, b, 3, 3);
}

void yf_mat4_sub(yf_mat4_t dst, const yf_mat4_t a, const yf_mat4_t b)
{
    YF_MAT_SUB(dst, a, b, 4, 4);
}

void yf_mat2_add(yf_mat2_t dst, const yf_mat2_t a, const yf_mat2_t b)
{
    YF_MAT_ADD(dst, a, b, 2, 2);
}

void yf_mat3_add(yf_mat3_t dst, const yf_mat3_t a, const yf_mat3_t b)
{
    YF_MAT_ADD(dst, a, b, 3, 3);
}

void yf_mat4_add(yf_mat4_t dst, const yf_mat4_t a, const yf_mat4_t b)
{
    YF_MAT_ADD(dst, a, b, 4, 4);
}

void yf_mat2_mul(yf_mat2_t dst, const yf_mat2_t a, const yf_mat2_t b)
{
    YF_MAT_MUL(dst, a, b, 2, 2, 2);
}

void yf_mat3_mul(yf_mat3_t dst, const yf_mat3_t a, const yf_mat3_t b)
{
    YF_MAT_MUL(dst, a, b, 3, 3, 3);
}

void yf_mat4_mul(yf_mat4_t dst, const yf_mat4_t a, const yf_mat4_t b)
{
    YF_MAT_MUL(dst, a, b, 4, 4, 4);
}

void yf_mat2_mulv(yf_vec2_t dst, const yf_mat2_t m, const yf_vec2_t v)
{
    YF_MAT_MULV(dst, m, v, 2);
}

void yf_mat3_mulv(yf_vec3_t dst, const yf_mat3_t m, const yf_vec3_t v)
{
    YF_MAT_MULV(dst, m, v, 3);
}

void yf_mat4_mulv(yf_vec4_t dst, const yf_mat4_t m, const yf_vec4_t v)
{
    YF_MAT_MULV(dst, m, v, 4);
}

void yf_mat2_inv(yf_mat2_t dst, const yf_mat2_t m)
{
    const float idet = 1.0f / (m[0] * m[3] - m[1] * m[2]);
    dst[0] = +m[3] * idet;
    dst[1] = +m[1] * idet;
    dst[2] = -m[2] * idet;
    dst[3] = +m[0] * idet;
}

void yf_mat3_inv(yf_mat3_t dst, const yf_mat3_t m)
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

void yf_mat4_inv(yf_mat4_t dst, const yf_mat4_t m)
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

void yf_mat3_rotx(yf_mat3_t m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat3_iden(m);
    m[4] = c;
    m[5] = s;
    m[7] = -s;
    m[8] = c;
}

void yf_mat3_roty(yf_mat3_t m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat3_iden(m);
    m[0] = c;
    m[2] = -s;
    m[6] = s;
    m[8] = c;
}

void yf_mat3_rotz(yf_mat3_t m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat3_iden(m);
    m[0] = c;
    m[1] = s;
    m[3] = -s;
    m[4] = c;
}

void yf_mat4_rotx(yf_mat4_t m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat4_iden(m);
    m[5] = c;
    m[6] = s;
    m[9] = -s;
    m[10] = c;
}

void yf_mat4_roty(yf_mat4_t m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat4_iden(m);
    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;
}

void yf_mat4_rotz(yf_mat4_t m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    yf_mat4_iden(m);
    m[0] = c;
    m[1] = s;
    m[4] = -s;
    m[5] = c;
}

void yf_mat3_rot(yf_mat3_t m, float angle, const yf_vec3_t axis)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    const float one_minus_c = 1.0f - c;
    yf_vec3_t v;
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

void yf_mat4_rot(yf_mat4_t m, float angle, const yf_vec3_t axis)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    const float one_minus_c = 1.0f - c;
    yf_vec3_t v;
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

void yf_mat3_rotq(yf_mat3_t m, const yf_vec4_t q)
{
    yf_vec4_t u;
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

void yf_mat4_rotq(yf_mat4_t m, const yf_vec4_t q)
{
    yf_vec4_t u;
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

void yf_mat3_scale(yf_mat3_t m, float sx, float sy, float sz)
{
    memset(m, 0, sizeof(yf_mat3_t));
    m[0] = sx;
    m[4] = sy;
    m[8] = sz;
}

void yf_mat4_scale(yf_mat4_t m, float sx, float sy, float sz)
{
    memset(m, 0, sizeof(yf_mat4_t));
    m[0] = sx;
    m[5] = sy;
    m[10] = sz;
    m[15] = 1.0f;
}

void yf_mat4_xlate(yf_mat4_t m, float tx, float ty, float tz)
{
    yf_mat4_iden(m);
    m[12] = tx;
    m[13] = ty;
    m[14] = tz;
}

void yf_mat4_lookat(yf_mat4_t m, const yf_vec3_t eye, const yf_vec3_t center,
                    const yf_vec3_t up)
{
    yf_vec3_t f, s, u;
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

void yf_mat4_persp(yf_mat4_t m, float yfov, float aspect, float znear,
                   float zfar)
{
    const float ct = 1.0f / tanf(yfov * 0.5f);
    memset(m, 0, sizeof(yf_mat4_t));
    m[0] = ct / aspect;
    m[5] = ct;
    m[10] = (zfar + znear) / (znear - zfar);
    m[11] = -1.0f;
    m[14] = (2.0f * zfar * znear) / (znear - zfar);
}

void yf_mat4_infpersp(yf_mat4_t m, float yfov, float aspect, float znear)
{
    const float ct = 1.0f / tanf(yfov * 0.5f);
    memset(m, 0, sizeof(yf_mat4_t));
    m[0] = ct / aspect;
    m[5] = ct;
    m[10] = m[11] = -1.0f;
    m[14] = -2.0f * znear;
}

void yf_mat4_ortho(yf_mat4_t m, float xmag, float ymag, float znear,
                   float zfar)
{
    memset(m, 0, sizeof(yf_mat4_t));
    m[0] = 1.0f / xmag;
    m[5] = 1.0f / ymag;
    m[10] = 2.0f / (znear - zfar);
    m[14] = (zfar + znear) / (znear - zfar);
    m[15] = 1.0f;
}
