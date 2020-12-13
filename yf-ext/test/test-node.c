/*
 * YF
 * test-node.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "yf-node.h"

/* Iteration callback for node. */
static int do_each_node(YF_node node, void *arg) {
  int r = (long)arg;
  printf("@%p (%d)\n", (void *)node, r);
  return r;
}

/* Tests node functionality. */
/* TODO: Check that values are correct, don't just print them. */
int yf_test_node(void) {
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

  printf("descends: %d, %d, %d, %d, %d, %d, %d\n", yf_node_descends(n2, n1),
      yf_node_descends(n3, n1), yf_node_descends(n4, n1),
      yf_node_descends(n5, n1), yf_node_descends(n5, n2),
      yf_node_descends(n5, n3), yf_node_descends(n1, n5));

  printf("isleaf: %d, %d, %d, %d, %d\n", yf_node_isleaf(n1), yf_node_isleaf(n2),
      yf_node_isleaf(n3), yf_node_isleaf(n4), yf_node_isleaf(n5));

  printf("getlen: %lu, %lu, %lu, %lu, %lu\n", yf_node_getlen(n1),
      yf_node_getlen(n2), yf_node_getlen(n3), yf_node_getlen(n4),
      yf_node_getlen(n5));

  puts("drop: _");
  yf_node_drop(n3);

  printf("traverse (n1):\n");
  yf_node_traverse(n1, do_each_node, 0);

  printf("descends: %d, %d, %d, %d, %d, %d, %d\n", yf_node_descends(n2, n1),
      yf_node_descends(n3, n1), yf_node_descends(n4, n1),
      yf_node_descends(n5, n1), yf_node_descends(n5, n2),
      yf_node_descends(n5, n3), yf_node_descends(n1, n5));

  printf("getlen: %lu, %lu, %lu, %lu, %lu\n", yf_node_getlen(n1),
      yf_node_getlen(n2), yf_node_getlen(n3), yf_node_getlen(n4),
      yf_node_getlen(n5));

  printf("isleaf: %d, %d, %d, %d, %d\n", yf_node_isleaf(n1), yf_node_isleaf(n2),
      yf_node_isleaf(n3), yf_node_isleaf(n4), yf_node_isleaf(n5));

  puts("prune: _");
  yf_node_prune(n1);

  printf("descends: %d, %d, %d, %d, %d, %d, %d\n", yf_node_descends(n2, n1),
      yf_node_descends(n3, n1), yf_node_descends(n4, n1),
      yf_node_descends(n5, n1), yf_node_descends(n5, n2),
      yf_node_descends(n5, n3), yf_node_descends(n1, n5));

  printf("isleaf: %d, %d, %d, %d, %d\n", yf_node_isleaf(n1),
      yf_node_isleaf(n2), yf_node_isleaf(n3), yf_node_isleaf(n4),
      yf_node_isleaf(n5));

  printf("getlen: %lu, %lu, %lu, %lu, %lu\n", yf_node_getlen(n1),
      yf_node_getlen(n2), yf_node_getlen(n3), yf_node_getlen(n4),
      yf_node_getlen(n5));

  yf_node_deinit(n1);
  yf_node_deinit(n2);
  yf_node_deinit(n3);
  yf_node_deinit(n4);
  yf_node_deinit(n5);

  return 0;
}
