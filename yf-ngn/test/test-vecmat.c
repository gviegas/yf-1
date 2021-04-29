/*
 * YF
 * test-vecmat.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>

#include "yf-vector.h"
#include "yf-matrix.h"

#define YF_PVEC(v, n) do { \
  printf("<"#v"> "); \
  for (int i = 0; i < n; ++i) \
    printf("%.4f ", v[i]); \
  puts("\n"); } while (0)

#define YF_PMAT(m, n) do { \
  puts("<"#m">"); \
  for (int i = 0; i < n; ++i) { \
    for (int j = 0; j < n; ++j) \
      printf("%.4f ", m[j*n+i]); \
    puts(""); \
  } \
  puts(""); } while (0)

/* Tests vector and matrix functionality. */
int yf_test_vecmat(void)
{
  YF_vec3 v = {1.0f, 0.0f, 0.0f};
  YF_vec3 u = {0.0f, 1.0f, 0.0f};
  YF_vec3 t;
  yf_vec3_cross(t, u, v);

  YF_PVEC(v, 3);
  YF_PVEC(u, 3);
  YF_PVEC(t, 3);

  if (v[0] != 1 || v[1] != 0 || v[2] != 0 ||
      u[0] != 0 || u[1] != 1 || v[2] != 0 ||
      t[0] != 0 || t[1] != 0 || t[2] != -1)
    return -1;

  YF_vec4 a = {2.0f, 4.0f, 2.0f, 5.0f};
  YF_float dot = yf_vec4_dot(a, a);
  YF_float len = yf_vec4_len(a);

  YF_PVEC(a, 4);
  printf("<dot a,a> %.4f\n\n", dot);
  printf("<len a> %.4f\n\n", len);

  if (dot != 49.0f || len != 7.0f)
    return -1;

  YF_vec4 b;
  yf_vec4_norm(b, a);

  YF_PVEC(b, 4);

  if (b[0] != a[0]/len || b[1] != a[1]/len || b[2] != a[2]/len ||
      b[3] != a[3]/len)
    return -1;

  YF_mat4 m0, m1;
  yf_mat4_iden(m0);
  yf_mat4_inv(m1, m0);

  if (memcmp(m0, m1, sizeof m1) != 0)
    return -1;

  YF_PMAT(m0, 4);
  YF_PMAT(m1, 4);

  YF_mat4 m2;
  yf_mat4_xlate(m2, 13.0f, -1.0f, -25.0f);

  if (m2[12] != 13.0f || m2[13] != -1.0f || m2[14] != -25.0f || m2[15] != 1.0f)
    return -1;

  YF_PMAT(m2, 4);

  YF_mat3 m3, m4, m5;
  yf_mat3_rotx(m3, 3.141593f);
  yf_mat3_roty(m4, 3.141593f);
  yf_mat3_rotz(m5, 3.141593f);

  YF_PMAT(m3, 3);
  YF_PMAT(m4, 3);
  YF_PMAT(m5, 3);

  YF_mat4 m6, m7;
  yf_mat4_roty(m6, 3.141593f);
  yf_mat4_mul(m7, m6, m2);

  YF_PMAT(m6, 4);
  YF_PMAT(m7, 4);

  YF_mat2 m8, m9;
  yf_mat2_set(m8, 99.9999f);
  yf_mat2_copy(m9, m8);

  YF_PMAT(m8, 2);
  YF_PMAT(m9, 2);

  if (memcmp(m8, m9, sizeof m9) != 0)
    return -1;

  YF_vec4 c, d;
  yf_vec4_set(c, 2.0f);
  yf_mat4_mulv(d, m2, c);

  YF_PVEC(c, 4);
  YF_PVEC(d, 4);

  if (d[0] != m2[0]*c[0]+m2[12]*c[3] || d[1] != m2[5]*c[1]+m2[13]*c[3] ||
      d[2] != m2[10]*c[2]+m2[14]*c[3] || d[3] != m2[15]*c[3])
    return -1;

  YF_vec3 e = {-3.0f, -100.2f, 5.01f};
  YF_vec2 f = {-3.0f, -100.2f};
  YF_vec4 g;
  yf_vec4_set(g, 0.0f);
  int iseq = yf_vec2_iseq(e, f);
  int iszero = yf_vec4_iszero(g);

  YF_PVEC(e, 3);
  YF_PVEC(f, 2);
  YF_PVEC(g, 4);
  printf("<iseq e,f> %s\n\n", iseq ? "yes" : "no");
  printf("<iszero g> %s\n", iszero ? "yes" : "no");

  if (!iseq || !iszero)
    return -1;

  return 0;
}
