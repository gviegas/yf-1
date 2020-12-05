/*
 * YF
 * test1.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <yf/core/yf-core.h>

#include "yf-vector.h"
#include "yf-matrix.h"
#include "yf-node.h"
#include "test.h"

/* Iteration callback for node. */
static int do_each_node(YF_node node, void *arg);

/* Tests vector & matrix functionality. */
static int test_vecmat(void);

/* Tests node functionality. */
static int test_node(void);

int yf_test_1(void) {
  int r;
  if ((r = test_vecmat()) != 0) {}
  else if ((r = test_node()) != 0) {}
  return r;
}

static int do_each_node(YF_node node, void *arg) {
  int r = (int)arg;
  printf("@%p (%d)\n", (void *)node, r);
  return r;
}

static int test_vecmat(void) {
#define PVEC(v, n) do { \
  puts("=vec="); \
  for (int i = 0; i < n; ++i) \
    printf("%.4f ", v[i]); \
  puts(""); } while (0)

#define PMAT(m, n) do { \
  puts("=mat="); \
  for (int i = 0; i < n; ++i) { \
    for (int j = 0; j < n; ++j) \
      printf("%.4f ", m[j*n+i]); \
    puts(""); \
  } } while (0)

  YF_vec3 v = {1.0, 0.0, 0.0};
  YF_vec3 u = {0.0, 1.0, 0.0};
  YF_vec3 t = {0};
  yf_vec3_cross(t, u, v);
  PVEC(v, 3);
  PVEC(u, 3);
  PVEC(t, 3);
  YF_vec4 a = {9.3, 6.45, 41.0, 0.0};
  printf("dot #%.4f, len #%.4f\n", yf_vec4_dot(a, a), yf_vec4_len(a));
  PVEC(a, 4);
  yf_vec4_normi(a);
  PVEC(a, 4);

  YF_mat4 m1, m2;
  yf_mat4_iden(m1);
  yf_mat4_xlate(m2, 13.0f, 4.503f, 25.02f);
  PMAT(m1, 4);
  PMAT(m2, 4);
  YF_mat3 m3, m4, m5;
  yf_mat3_rotx(m3, 3.141593f);
  yf_mat3_roty(m4, 3.141593f * 1.25f);
  yf_mat3_rotz(m5, 3.141593f * 0.65f);
  PMAT(m3, 3);
  PMAT(m4, 3);
  PMAT(m5, 3);
  YF_mat4 m6, m7;
  yf_mat4_roty(m6, 3.141593f);
  yf_mat4_mul(m7, m6, m2);
  YF_mat2 m8, m9;
  yf_mat2_set(m8, 99.9999f);
  yf_mat2_copy(m9, m8);
  PMAT(m8, 2);
  PMAT(m9, 2);
  YF_vec4 b, c;
  yf_vec4_set(b, 1.2f);
  yf_mat4_mulv(c, m2, b);
  PVEC(b, 4);
  PVEC(c, 4);

#undef PVEC
#undef PMAT
  puts("----");
  return 0;
}

static int test_node(void) {
  YF_node n1, n2, n3, n4, n5;
  n1 = yf_node_init();
  n2 = yf_node_init();
  n3 = yf_node_init();
  n4 = yf_node_init();
  n5 = yf_node_init();
  assert(n1 && n2 && n3 && n4 && n5);
  puts("=node=");
  puts("insert: n1<>n2, n1<>n3, n1<>n4, n3<>n5");
  yf_node_insert(n1, n2);
  yf_node_insert(n1, n3);
  yf_node_insert(n1, n4);
  yf_node_insert(n3, n5);
  printf("n1 is @%p\n", (void *)n1);
  printf("n2 is @%p\n", (void *)n2);
  printf("n3 is @%p\n", (void *)n3);
  printf("n4 is @%p\n", (void *)n4);
  printf("n5 is @%p\n", (void *)n5);
  printf("traverse (n1):\n");
  yf_node_traverse(n1, do_each_node, 0);
  printf("traverse (n2):\n");
  yf_node_traverse(n2, do_each_node, 0);
  printf("traverse (n3):\n");
  yf_node_traverse(n3, do_each_node, 0);
  printf("traverse (n4):\n");
  yf_node_traverse(n4, do_each_node, 0);
  printf("traverse (n5):\n");
  yf_node_traverse(n5, do_each_node, 0);
  printf(
    "descends: %d, %d, %d, %d, %d, %d, %d\n",
    yf_node_descends(n2, n1),
    yf_node_descends(n3, n1),
    yf_node_descends(n4, n1),
    yf_node_descends(n5, n1),
    yf_node_descends(n5, n2),
    yf_node_descends(n5, n3),
    yf_node_descends(n1, n5));
  printf(
    "isleaf: %d, %d, %d, %d, %d\n",
    yf_node_isleaf(n1),
    yf_node_isleaf(n2),
    yf_node_isleaf(n3),
    yf_node_isleaf(n4),
    yf_node_isleaf(n5));
  printf(
    "getlen: %lu, %lu, %lu, %lu, %lu\n",
    yf_node_getlen(n1),
    yf_node_getlen(n2),
    yf_node_getlen(n3),
    yf_node_getlen(n4),
    yf_node_getlen(n5));
  puts("drop: _");
  yf_node_drop(n3);
  printf("traverse (n1):\n");
  yf_node_traverse(n1, do_each_node, 0);
  printf(
    "descends: %d, %d, %d, %d, %d, %d, %d\n",
    yf_node_descends(n2, n1),
    yf_node_descends(n3, n1),
    yf_node_descends(n4, n1),
    yf_node_descends(n5, n1),
    yf_node_descends(n5, n2),
    yf_node_descends(n5, n3),
    yf_node_descends(n1, n5));
  printf(
    "getlen: %lu, %lu, %lu, %lu, %lu\n",
    yf_node_getlen(n1),
    yf_node_getlen(n2),
    yf_node_getlen(n3),
    yf_node_getlen(n4),
    yf_node_getlen(n5));
  printf(
    "isleaf: %d, %d, %d, %d, %d\n",
    yf_node_isleaf(n1),
    yf_node_isleaf(n2),
    yf_node_isleaf(n3),
    yf_node_isleaf(n4),
    yf_node_isleaf(n5));
  puts("prune: _");
  yf_node_prune(n1);
  printf(
    "descends: %d, %d, %d, %d, %d, %d, %d\n",
    yf_node_descends(n2, n1),
    yf_node_descends(n3, n1),
    yf_node_descends(n4, n1),
    yf_node_descends(n5, n1),
    yf_node_descends(n5, n2),
    yf_node_descends(n5, n3),
    yf_node_descends(n1, n5));
  printf(
    "isleaf: %d, %d, %d, %d, %d\n",
    yf_node_isleaf(n1),
    yf_node_isleaf(n2),
    yf_node_isleaf(n3),
    yf_node_isleaf(n4),
    yf_node_isleaf(n5));
  printf(
    "getlen: %lu, %lu, %lu, %lu, %lu\n",
    yf_node_getlen(n1),
    yf_node_getlen(n2),
    yf_node_getlen(n3),
    yf_node_getlen(n4),
    yf_node_getlen(n5));
  yf_node_deinit(n1);
  yf_node_deinit(n2);
  yf_node_deinit(n3);
  yf_node_deinit(n4);
  yf_node_deinit(n5);
  puts("----");
  return 0;
}
