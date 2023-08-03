/*
 * YF
 * test-vector.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "test.h"
#include "yf-vector.h"

#define YF_PRINTVEC(v, n) do { \
    printf("\nvec"#n" "#v"\n [ "); \
    for (int i = 0; i < n; i++) \
        printf("%.4f ", v[i]); \
    puts("]"); } while (0)

/* Tests vector. */
/* TODO: More tests. */
int yf_test_vector(void)
{
    char s[256];

    yf_vec4_t a = {2.0f, 4.0f, 2.0f, 5.0f};
    YF_PRINTVEC(a, 4);

    float dot = yf_vec4_dot(a, a);
    snprintf(s, sizeof s, "%.4f", dot);
    YF_TEST_PRINT("dot", "a, a", s);

    float len = yf_vec4_len(a);
    snprintf(s, sizeof s, "%.4f", len);
    YF_TEST_PRINT("len", "a", s);

    if (dot != 49.0f || len != 7.0f)
        return -1;

    yf_vec4_t b;
    yf_vec4_norm(b, a);
    YF_TEST_PRINT("norm", "b, a", "");
    YF_PRINTVEC(b, 4);

    if (b[0] != a[0]/len || b[1] != a[1]/len || b[2] != a[2]/len ||
        b[3] != a[3]/len)
        return -1;

    yf_vec3_t c = {1.0f, 0.0f, 0.0f};
    yf_vec3_t d = {0.0f, 1.0f, 0.0f};
    yf_vec3_t e;
    yf_vec3_cross(e, d, c);
    YF_PRINTVEC(c, 3);
    YF_PRINTVEC(d, 3);
    YF_TEST_PRINT("cross", "e, d, c", "");
    YF_PRINTVEC(e, 3);

    if (c[0] != 1 || c[1] != 0 || c[2] != 0 ||
        d[0] != 0 || d[1] != 1 || d[2] != 0 ||
        e[0] != 0 || e[1] != 0 || e[2] != -1)
        return -1;

    yf_vec2_t f = {-1.0f, -100.2f};
    yf_vec3_t g = {-1.0f, -100.2f, 5.9987f};
    int iseq = yf_vec2_iseq(f, g);
    YF_PRINTVEC(f, 2);
    YF_PRINTVEC(g, 3);
    YF_TEST_PRINT("iseq", "f, g", (iseq ? "yes" : "no"));

    if (!iseq)
        return -1;

    int iszero = yf_vec3_iszero(g);
    YF_TEST_PRINT("iszero", "g", (iszero ? "yes" : "no"));

    if (iszero)
        return -1;

    yf_vec3_set(g, 0.0f);
    iszero = yf_vec3_iszero(g);
    YF_TEST_PRINT("set", "g, 0.0f", "");
    YF_PRINTVEC(g, 3);
    YF_TEST_PRINT("iszero", "g", (iszero ? "yes" : "no"));

    if (!iszero)
        return -1;

    iseq = yf_vec2_iseq(g, f);
    YF_TEST_PRINT("iseq", "g, f", (iseq ? "yes" : "no"));

    if (iseq)
        return -1;

    yf_vec2_copy(f, g);
    YF_TEST_PRINT("copy", "f, g", "");
    YF_PRINTVEC(f, 2);
    iseq = yf_vec2_iseq(g, f);
    YF_TEST_PRINT("iseq", "g, f", (iseq ? "yes" : "no"));

    if (!iseq)
        return -1;

    return 0;
}
