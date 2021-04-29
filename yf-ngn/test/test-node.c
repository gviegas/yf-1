/*
 * YF
 * test-node.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "yf-node.h"

/* Iteration callback for 'node_traverse()'. */
static int do_each_node(YF_node node, void *arg)
{
  char name[16];
  yf_node_getname(node, name, sizeof name);
  int r = (long)arg;
  printf("@%p, '%s', %d\n", (void *)node, name, r);
  return r;
}

/* Tests node functionality. */
int yf_test_node(void)
{
  YF_node n1, n2, n3, n4, n5;
  int chk[7];
  size_t len[5];

  n1 = yf_node_init();
  n2 = yf_node_init();
  n3 = yf_node_init();
  n4 = yf_node_init();
  n5 = yf_node_init();

  puts("<init>");
  printf("n1 is @%p\n", (void *)n1);
  printf("n2 is @%p\n", (void *)n2);
  printf("n3 is @%p\n", (void *)n3);
  printf("n4 is @%p\n", (void *)n4);
  printf("n5 is @%p\n", (void *)n5);

  if (n1 == NULL || n2 == NULL || n3 == NULL || n4 == NULL || n5 == NULL)
    return -1;

  yf_node_insert(n1, n2);
  yf_node_insert(n1, n3);
  yf_node_insert(n1, n4);
  yf_node_insert(n3, n5);

  puts("\n<insert> n1-n2, n1-n3, n1-n4, n3-n5");

  yf_node_setname(n1, "node1");
  yf_node_setname(n2, "node2");
  yf_node_setname(n3, "node3");
  yf_node_setname(n4, "node4");
  yf_node_setname(n5, "node5");

  printf("\n<traverse> (n1):\n");
  yf_node_traverse(n1, do_each_node, 0);
  printf("\n<traverse> (n2):\n");
  yf_node_traverse(n2, do_each_node, 0);
  printf("\n<traverse> (n3):\n");
  yf_node_traverse(n3, do_each_node, 0);
  printf("\n<traverse> (n4):\n");
  yf_node_traverse(n4, do_each_node, 0);
  printf("\n<traverse> (n5):\n");
  yf_node_traverse(n5, do_each_node, 0);

#define YF_DESCCHK() do { \
  chk[0] = yf_node_descends(n2, n1); \
  chk[1] = yf_node_descends(n3, n1); \
  chk[2] = yf_node_descends(n4, n1); \
  chk[3] = yf_node_descends(n5, n1); \
  chk[4] = yf_node_descends(n5, n2); \
  chk[5] = yf_node_descends(n5, n3); \
  chk[6] = yf_node_descends(n1, n5); \
  \
  puts("\n<descends>"); \
  printf("n2,n1: %s\n", chk[0] ? "yes" : "no"); \
  printf("n3,n1: %s\n", chk[1] ? "yes" : "no"); \
  printf("n4,n1: %s\n", chk[2] ? "yes" : "no"); \
  printf("n5,n1: %s\n", chk[3] ? "yes" : "no"); \
  printf("n5,n2: %s\n", chk[4] ? "yes" : "no"); \
  printf("n5,n3: %s\n", chk[5] ? "yes" : "no"); \
  printf("n1,n5: %s\n", chk[6] ? "yes" : "no"); } while (0)

#define YF_LEAFCHK() do { \
  chk[0] = yf_node_isleaf(n1); \
  chk[1] = yf_node_isleaf(n2); \
  chk[2] = yf_node_isleaf(n3); \
  chk[3] = yf_node_isleaf(n4); \
  chk[4] = yf_node_isleaf(n5); \
  \
  puts("\n<isleaf>"); \
  printf("n1: %s\n", chk[0] ? "yes" : "no"); \
  printf("n2: %s\n", chk[1] ? "yes" : "no"); \
  printf("n3: %s\n", chk[2] ? "yes" : "no"); \
  printf("n4: %s\n", chk[3] ? "yes" : "no"); \
  printf("n5: %s\n", chk[4] ? "yes" : "no"); } while (0)

#define YF_LENCHK() do { \
  len[0] = yf_node_getlen(n1); \
  len[1] = yf_node_getlen(n2); \
  len[2] = yf_node_getlen(n3); \
  len[3] = yf_node_getlen(n4); \
  len[4] = yf_node_getlen(n5); \
  \
  puts("\n<getlen>"); \
  printf("n1: %lu\n", len[0]); \
  printf("n2: %lu\n", len[1]); \
  printf("n3: %lu\n", len[2]); \
  printf("n4: %lu\n", len[3]); \
  printf("n5: %lu\n", len[4]); } while (0)

  YF_DESCCHK();
  if (!chk[0] || !chk[1] || !chk[2] || !chk[3] || chk[4] || !chk[5] || chk[6])
    return -1;
  YF_LEAFCHK();
  if (chk[0] || !chk[1] || chk[2] || !chk[3] || !chk[4])
    return -1;
  YF_LENCHK();
  if (len[0] != 5 || len[1] != 1 || len[2] != 2 || len[3] != 1 || len[4] != 1)
    return -1;

  if (yf_node_cmpname(n2, "node2") != 0)
    return -1;
  yf_node_setname(n2, "NODE2");
  if (yf_node_cmpname(n2, "node2") == 0)
    return -1;

  if (yf_node_cmpname(n5, "node-5") == 0)
    return -1;
  yf_node_setname(n5, "node-5");
  if (yf_node_cmpname(n5, "node-5") != 0)
    return -1;

  puts("\n<drop> (n3)");
  yf_node_drop(n3);

  printf("\n<traverse> (n1):\n");
  yf_node_traverse(n1, do_each_node, 0);

  printf("\n<traverse> (n3):\n");
  yf_node_traverse(n3, do_each_node, 0);

  YF_DESCCHK();
  if (!chk[0] || chk[1] || !chk[2] || chk[3] || chk[4] || !chk[5] || chk[6])
    return -1;
  YF_LEAFCHK();
  if (chk[0] || !chk[1] || chk[2] || !chk[3] || !chk[4])
    return -1;
  YF_LENCHK();
  if (len[0] != 3 || len[1] != 1 || len[2] != 2 || len[3] != 1 || len[4] != 1)
    return -1;

  puts("\n<prune> (n1)");
  yf_node_prune(n1);

  printf("\n<traverse> (n1):\n");
  yf_node_traverse(n1, do_each_node, 0);

  printf("\n<traverse> (n3):\n");
  yf_node_traverse(n3, do_each_node, 0);

  YF_DESCCHK();
  if (chk[0] || chk[1] || chk[2] || chk[3] || chk[4] || !chk[5] || chk[6])
    return -1;
  YF_LEAFCHK();
  if (!chk[0] || !chk[1] || chk[2] || !chk[3] || !chk[4])
    return -1;
  YF_LENCHK();
  if (len[0] != 1 || len[1] != 1 || len[2] != 2 || len[3] != 1 || len[4] != 1)
    return -1;

  yf_node_deinit(n1);
  yf_node_deinit(n2);
  yf_node_deinit(n3);
  yf_node_deinit(n4);
  yf_node_deinit(n5);

  return 0;
}
