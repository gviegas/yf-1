/*
 * YF
 * vector.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <string.h>
#include <math.h>

#include "yf-vector.h"

#define YF_FLT_ISEQ(a, b) (fabsf((a)-(b)) < 1e-6f)

#define YF_VEC_ISEQ(r, a, b, n) do { \
    for (unsigned i = 0; i < (n); i++) { \
        if (!(r = YF_FLT_ISEQ((a)[i], (b)[i]))) \
            break; \
    } } while (0);

#define YF_VEC_SET(v, s, n) do { \
    for (unsigned i = 0; i < (n); i++) \
        (v)[i] = s; } while (0)

#define YF_VEC_SUB(dst, a, b, n) do { \
    for (unsigned i = 0; i < (n); i++) \
        (dst)[i] = (a)[i] - (b)[i]; } while (0)

#define YF_VEC_ADD(dst, a, b, n) do { \
    for (unsigned i = 0; i < (n); i++) \
        (dst)[i] = (a)[i] + (b)[i]; } while (0)

#define YF_VEC_MULS(dst, v, s, n) do { \
    for (unsigned i = 0; i < (n); i++) \
        (dst)[i] = (v)[i] * (s); } while (0)

#define YF_VEC_DOT(r, a, b, n) do { \
    r = 0.0f; \
    for (unsigned i = 0; i < (n); i++) \
        r += (a)[i] * (b)[i]; } while (0)

int yf_vec2_iszero(const yf_vec2_t v)
{
    static const yf_vec2_t zero = {0.0f, 0.0f};
    int r;
    YF_VEC_ISEQ(r, v, zero, 2);
    return r;
}

int yf_vec3_iszero(const yf_vec3_t v)
{
    static const yf_vec3_t zero = {0.0f, 0.0f, 0.0f};
    int r;
    YF_VEC_ISEQ(r, v, zero, 3);
    return r;
}

int yf_vec4_iszero(const yf_vec4_t v)
{
    static const yf_vec4_t zero = {0.0f, 0.0f, 0.0f, 0.0f};
    int r;
    YF_VEC_ISEQ(r, v, zero, 4);
    return r;
}

int yf_vec2_iseq(const yf_vec2_t a, const yf_vec2_t b)
{
    int r;
    YF_VEC_ISEQ(r, a, b, 2);
    return r;
}

int yf_vec3_iseq(const yf_vec3_t a, const yf_vec3_t b)
{
    int r;
    YF_VEC_ISEQ(r, a, b, 3);
    return r;
}

int yf_vec4_iseq(const yf_vec4_t a, const yf_vec4_t b)
{
    int r;
    YF_VEC_ISEQ(r, a, b, 4);
    return r;
}

void yf_vec2_set(yf_vec2_t v, float s)
{
    YF_VEC_SET(v, s, 2);
}

void yf_vec3_set(yf_vec3_t v, float s)
{
    YF_VEC_SET(v, s, 3);
}

void yf_vec4_set(yf_vec4_t v, float s)
{
    YF_VEC_SET(v, s, 4);
}

void yf_vec2_copy(yf_vec2_t dst, const yf_vec2_t v)
{
    memcpy(dst, v, sizeof(yf_vec2_t));
}

void yf_vec3_copy(yf_vec3_t dst, const yf_vec3_t v)
{
    memcpy(dst, v, sizeof(yf_vec3_t));
}

void yf_vec4_copy(yf_vec4_t dst, const yf_vec4_t v)
{
    memcpy(dst, v, sizeof(yf_vec4_t));
}

void yf_vec2_sub(yf_vec2_t dst, const yf_vec2_t a, const yf_vec2_t b)
{
    YF_VEC_SUB(dst, a, b, 2);
}

void yf_vec3_sub(yf_vec3_t dst, const yf_vec3_t a, const yf_vec3_t b)
{
    YF_VEC_SUB(dst, a, b, 3);
}

void yf_vec4_sub(yf_vec4_t dst, const yf_vec4_t a, const yf_vec4_t b)
{
    YF_VEC_SUB(dst, a, b, 4);
}

void yf_vec2_subi(yf_vec2_t dst, const yf_vec2_t v)
{
    YF_VEC_SUB(dst, dst, v, 2);
}

void yf_vec3_subi(yf_vec3_t dst, const yf_vec3_t v)
{
    YF_VEC_SUB(dst, dst, v, 3);
}

void yf_vec4_subi(yf_vec4_t dst, const yf_vec4_t v)
{
    YF_VEC_SUB(dst, dst, v, 4);
}

void yf_vec2_add(yf_vec2_t dst, const yf_vec2_t a, const yf_vec2_t b)
{
    YF_VEC_ADD(dst, a, b, 2);
}

void yf_vec3_add(yf_vec3_t dst, const yf_vec3_t a, const yf_vec3_t b)
{
    YF_VEC_ADD(dst, a, b, 3);
}

void yf_vec4_add(yf_vec4_t dst, const yf_vec4_t a, const yf_vec4_t b)
{
    YF_VEC_ADD(dst, a, b, 4);
}

void yf_vec2_addi(yf_vec2_t dst, const yf_vec2_t v)
{
    YF_VEC_ADD(dst, dst, v, 2);
}

void yf_vec3_addi(yf_vec3_t dst, const yf_vec3_t v)
{
    YF_VEC_ADD(dst, dst, v, 3);
}

void yf_vec4_addi(yf_vec4_t dst, const yf_vec4_t v)
{
    YF_VEC_ADD(dst, dst, v, 4);
}

void yf_vec2_muls(yf_vec2_t dst, const yf_vec2_t v, float s)
{
    YF_VEC_MULS(dst, v, s, 2);
}

void yf_vec3_muls(yf_vec3_t dst, const yf_vec3_t v, float s)
{
    YF_VEC_MULS(dst, v, s, 3);
}

void yf_vec4_muls(yf_vec4_t dst, const yf_vec4_t v, float s)
{
    YF_VEC_MULS(dst, v, s, 4);
}

void yf_vec2_mulsi(yf_vec2_t dst, float s)
{
    YF_VEC_MULS(dst, dst, s, 2);
}

void yf_vec3_mulsi(yf_vec3_t dst, float s)
{
    YF_VEC_MULS(dst, dst, s, 3);
}

void yf_vec4_mulsi(yf_vec4_t dst, float s)
{
    YF_VEC_MULS(dst, dst, s, 4);
}

float yf_vec2_dot(const yf_vec2_t a, const yf_vec2_t b)
{
    float r;
    YF_VEC_DOT(r, a, b, 2);
    return r;
}

float yf_vec3_dot(const yf_vec3_t a, const yf_vec3_t b)
{
    float r;
    YF_VEC_DOT(r, a, b, 3);
    return r;
}

float yf_vec4_dot(const yf_vec4_t a, const yf_vec4_t b)
{
    float r;
    YF_VEC_DOT(r, a, b, 4);
    return r;
}

float yf_vec2_len(const yf_vec2_t v)
{
    return sqrtf(yf_vec2_dot(v, v));
}

float yf_vec3_len(const yf_vec3_t v)
{
    return sqrtf(yf_vec3_dot(v, v));
}

float yf_vec4_len(const yf_vec4_t v)
{
    return sqrtf(yf_vec4_dot(v, v));
}

void yf_vec2_norm(yf_vec2_t dst, const yf_vec2_t v)
{
    const float s = 1.0f / yf_vec2_len(v);
    YF_VEC_MULS(dst, v, s, 2);
}

void yf_vec3_norm(yf_vec3_t dst, const yf_vec3_t v)
{
    const float s = 1.0f / yf_vec3_len(v);
    YF_VEC_MULS(dst, v, s, 3);
}

void yf_vec4_norm(yf_vec4_t dst, const yf_vec4_t v)
{
    const float s = 1.0f / yf_vec4_len(v);
    YF_VEC_MULS(dst, v, s, 4);
}

void yf_vec2_normi(yf_vec2_t v)
{
    yf_vec2_norm(v, v);
}

void yf_vec3_normi(yf_vec3_t v)
{
    yf_vec3_norm(v, v);
}

void yf_vec4_normi(yf_vec4_t v)
{
    yf_vec4_norm(v, v);
}

void yf_vec3_cross(yf_vec3_t dst, const yf_vec3_t a, const yf_vec3_t b)
{
    dst[0] = a[1] * b[2] - a[2] * b[1];
    dst[1] = a[2] * b[0] - a[0] * b[2];
    dst[2] = a[0] * b[1] - a[1] * b[0];
}

void yf_vec4_cross(yf_vec4_t dst, const yf_vec4_t a, const yf_vec4_t b)
{
    yf_vec3_cross(dst, a, b);
    dst[3] = 1.0f;
}

void yf_vec4_rotqx(yf_vec4_t q, float angle)
{
    const float a = angle * 0.5f;
    q[3] = cosf(a);
    q[0] = sinf(a);
    q[1] = q[2] = 0.0f;
}

void yf_vec4_rotqy(yf_vec4_t q, float angle)
{
    const float a = angle * 0.5f;
    q[3] = cosf(a);
    q[1] = sinf(a);
    q[0] = q[2] = 0.0f;
}

void yf_vec4_rotqz(yf_vec4_t q, float angle)
{
    const float a = angle * 0.5f;
    q[3] = cosf(a);
    q[2] = sinf(a);
    q[0] = q[1] = 0.0f;
}

void yf_vec4_rotq(yf_vec4_t q, float angle, const yf_vec3_t axis)
{
    yf_vec3_t v;
    yf_vec3_norm(v, axis);
    const float a = angle * 0.5f;
    const float c = cosf(a);
    const float s = sinf(a);
    q[3] = c;
    q[0] = s * v[0];
    q[1] = s * v[1];
    q[2] = s * v[2];
}

void yf_vec4_mulq(yf_vec4_t dst, const yf_vec4_t q1, const yf_vec4_t q2)
{
    yf_vec3_t v, u;
    yf_vec3_muls(v, q2, q1[3]);
    yf_vec3_muls(u, q1, q2[3]);
    yf_vec3_addi(v, u);
    yf_vec3_cross(u, q1, q2);
    yf_vec3_add(dst, v, u);
    dst[3] = q1[3] * q2[3] - yf_vec3_dot(q1, q2);
}

void yf_vec4_mulqi(yf_vec4_t dst, const yf_vec4_t q)
{
    const float r = dst[3] * q[3] - yf_vec3_dot(dst, q);
    yf_vec3_t v, u;
    yf_vec3_cross(v, dst, q);
    yf_vec3_mulsi(dst, q[3]);
    yf_vec3_muls(u, q, dst[3]);
    yf_vec3_addi(dst, u);
    yf_vec3_addi(dst, v);
    dst[3] = r;
}
