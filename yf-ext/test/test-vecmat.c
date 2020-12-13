/*
 * YF
 * test-vecmat.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "yf-vector.h"
#include "yf-matrix.h"

#define YF_PVEC(v, n) do { \
  puts("=vec="); \
  for (int i = 0; i < n; ++i) \
    printf("%.4f ", v[i]); \
  puts(""); } while (0)

#define YF_PMAT(m, n) do { \
  puts("=mat="); \
  for (int i = 0; i < n; ++i) { \
    for (int j = 0; j < n; ++j) \
      printf("%.4f ", m[j*n+i]); \
    puts(""); \
  } } while (0)

/* Tests vector & matrix functionality. */
/* TODO: Check if values are correct, don't just print them. */
int yf_test_vecmat(void) {
  YF_vec3 v = {1.0, 0.0, 0.0};
  YF_vec3 u = {0.0, 1.0, 0.0};
  YF_vec3 t = {0};
  yf_vec3_cross(t, u, v);
  YF_PVEC(v, 3);
  YF_PVEC(u, 3);
  YF_PVEC(t, 3);

  YF_vec4 a = {9.3, 6.45, 41.0, 0.0};
  printf("dot #%.4f, len #%.4f\n", yf_vec4_dot(a, a), yf_vec4_len(a));
  YF_PVEC(a, 4);
  yf_vec4_normi(a);
  YF_PVEC(a, 4);

  YF_mat4 m1, m2;
  yf_mat4_iden(m1);
  yf_mat4_xlate(m2, 13.0f, 4.503f, 25.02f);
  YF_PMAT(m1, 4);
  YF_PMAT(m2, 4);

  YF_mat3 m3, m4, m5;
  yf_mat3_rotx(m3, 3.141593f);
  yf_mat3_roty(m4, 3.141593f * 1.25f);
  yf_mat3_rotz(m5, 3.141593f * 0.65f);
  YF_PMAT(m3, 3);
  YF_PMAT(m4, 3);
  YF_PMAT(m5, 3);

  YF_mat4 m6, m7;
  yf_mat4_roty(m6, 3.141593f);
  YF_PMAT(m6, 4);
  yf_mat4_mul(m7, m6, m2);
  YF_PMAT(m7, 4);

  YF_mat2 m8, m9;
  yf_mat2_set(m8, 99.9999f);
  YF_PMAT(m8, 2);
  yf_mat2_copy(m9, m8);
  YF_PMAT(m9, 2);

  YF_vec4 b, c;
  yf_vec4_set(b, 1.2f);
  YF_PVEC(b, 4);
  yf_mat4_mulv(c, m2, b);
  YF_PVEC(c, 4);

  return 0;
}
