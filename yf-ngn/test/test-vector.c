/*
 * YF
 * test-vector.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "yf-vector.h"

#define YF_PRINTVEC(v, n) do { \
    printf("(vec"#n") '"#v"'\n [ "); \
    for (int i = 0; i < n; i++) \
        printf("%.4f ", v[i]); \
    puts("]\n"); } while (0)

/* Tests vector functionality. */
/* TODO: More tests. */
int yf_test_vector(void)
{
    YF_vec4 a = {2.0f, 4.0f, 2.0f, 5.0f};
    float dot = yf_vec4_dot(a, a);
    float len = yf_vec4_len(a);

    YF_PRINTVEC(a, 4);
    printf("(dot a,a)\n %.4f\n\n", dot);
    printf("(len a)\n %.4f\n\n", len);

    if (dot != 49.0f || len != 7.0f)
        return -1;

    YF_vec4 b;
    yf_vec4_norm(b, a);

    puts("(norm b,a)\n");
    YF_PRINTVEC(b, 4);

    if (b[0] != a[0]/len || b[1] != a[1]/len || b[2] != a[2]/len ||
        b[3] != a[3]/len)
        return -1;

    YF_vec3 c = {1.0f, 0.0f, 0.0f};
    YF_vec3 d = {0.0f, 1.0f, 0.0f};
    YF_vec3 e;
    yf_vec3_cross(e, d, c);

    YF_PRINTVEC(c, 3);
    YF_PRINTVEC(d, 3);
    puts("(cross e,d,c)\n");
    YF_PRINTVEC(e, 3);

    if (c[0] != 1 || c[1] != 0 || c[2] != 0 ||
        d[0] != 0 || d[1] != 1 || d[2] != 0 ||
        e[0] != 0 || e[1] != 0 || e[2] != -1)
        return -1;

    YF_vec2 f = {-1.0f, -100.2f};
    YF_vec3 g = {-1.0f, -100.2f, 5.9987f};
    int iseq = yf_vec2_iseq(f, g);

    YF_PRINTVEC(f, 2);
    YF_PRINTVEC(g, 3);
    printf("(iseq f,g)\n %s\n\n", iseq ? "yes" : "no");

    if (!iseq)
        return -1;

    int iszero = yf_vec3_iszero(g);

    printf("(iszero g)\n %s\n\n", iszero ? "yes" : "no");

    if (iszero)
        return -1;

    yf_vec3_set(g, 0.0f);
    iszero = yf_vec3_iszero(g);

    puts("(set g,0.0f)\n");
    YF_PRINTVEC(g, 3);
    printf("(iszero g)\n %s\n\n", iszero ? "yes" : "no");

    if (!iszero)
        return -1;

    iseq = yf_vec2_iseq(g, f);
    printf("(iseq g,f)\n %s\n\n", iseq ? "yes" : "no");

    if (iseq)
        return -1;

    yf_vec2_copy(f, g);
    puts("(copy f,g)\n");
    YF_PRINTVEC(f, 2);

    iseq = yf_vec2_iseq(g, f);
    printf("(iseq g,f)\n %s\n", iseq ? "yes" : "no");

    if (!iseq)
        return -1;

    return 0;
}
