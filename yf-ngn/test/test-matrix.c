/*
 * YF
 * test-matrix.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>

#include "yf-matrix.h"

#define YF_PRINTVEC(v, n) do { \
    printf("(vec"#n") '"#v"'\n [ "); \
    for (int i = 0; i < n; i++) \
        printf("%.4f ", v[i]); \
    puts("]\n"); } while (0)

#define YF_PRINTMAT(m, n) do { \
    printf("(mat"#n") '"#m"'\n"); \
    for (int i_ = 0; i_ < n; i_++) { \
        printf(" [ "); \
        for (int j_ = 0; j_ < n; j_++) \
            printf("%.4f ", m[j_*n+i_]); \
        printf("]\n"); \
    } \
    puts(""); } while (0)

/* Tests matrix functionality. */
/* TODO: Missing checks & more tests. */
int yf_test_matrix(void)
{
    YF_mat4 a;
    yf_mat4_iden(a);

    puts("(iden a)\n");
    YF_PRINTMAT(a, 4);

    YF_mat4 b;
    yf_mat4_inv(b, a);

    puts("(inv b,a)\n");
    YF_PRINTMAT(b, 4);

    if (memcmp(a, b, sizeof b) != 0)
        return -1;

    YF_mat4 c;
    yf_mat4_xlate(c, 13.0f, -1.0f, -25.0f);

    puts("(xlate c,13.0f,-1.0f,-25.0f)\n");
    YF_PRINTMAT(c, 4);

    if (c[12] != 13.0f || c[13] != -1.0f || c[14] != -25.0f || c[15] != 1.0f)
        return -1;

    YF_mat3 d;
    yf_mat3_rotx(d, 3.141593f);

    puts("(rotx d,3.141593f)\n");
    YF_PRINTMAT(d, 3);

    YF_mat3 e;
    yf_mat3_roty(e, 3.141593f);

    puts("(roty e,3.141593f)\n");
    YF_PRINTMAT(e, 3);

    YF_mat3 f;
    yf_mat3_rotz(f, 3.141593f);

    puts("(rotz f,3.141593f)\n");
    YF_PRINTMAT(f, 3);

    YF_mat4 g;
    yf_mat4_scale(g, 0.25f, 0.5f, 0.33f);

    if (g[0] != 0.25f || g[5] != 0.5f || g[10] != 0.33f || g[15] != 1.0f)
        return -1;

    puts("(scale g,0.25f,0.5f,0.33f)\n");
    YF_PRINTMAT(g, 4);

    YF_mat4 h;
    yf_mat4_mul(h, c, g);

    puts("(mul h,c,g)\n");
    YF_PRINTMAT(h, 4);

    YF_mat2 i;
    yf_mat2_set(i, 99.01f);

    puts("(set i,99.01f)\n");
    YF_PRINTMAT(i, 2);

    if (i[0] != 99.01f || i[1] != 99.01f || i[2] != 99.01f || i[3] != 99.01f)
        return -1;

    YF_mat2 j;
    yf_mat2_copy(j, i);

    puts("(copy j,i)\n");
    YF_PRINTMAT(j, 2);

    if (memcmp(i, j, sizeof j) != 0)
        return -1;

    YF_vec4 u, v;
    yf_vec4_set(u, 2.0f);
    yf_mat4_mulv(v, c, u);

    YF_PRINTVEC(u, 4);
    puts("(mulv v,c,u)\n");
    YF_PRINTVEC(v, 4);

    if (v[0] != c[0]*u[0]+c[12]*u[3] ||
        v[1] != c[5]*u[1]+c[13]*u[3] ||
        v[2] != c[10]*u[2]+c[14]*u[3] ||
        v[3] != c[15]*u[3])
        return -1;

    return 0;
}
