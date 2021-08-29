/*
 * YF
 * test-node.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "test.h"
#include "yf-node.h"

/* Iteration callback for 'node_traverse()'. */
static int do_each_node(YF_node node, void *arg)
{
    char name[16];
    size_t n = sizeof name;
    yf_node_getname(node, name, &n);
    int r = (long)arg;
    printf(" node: %p, name: %s (%zu), arg: %d\n", (void *)node, name, n, r);
    return r;
}

/* Tests node. */
int yf_test_node(void)
{
    YF_node n1, n2, n3, n4, n5;
    int chk[7];
    YF_node pnt[5];
    size_t len[5];
    char s[256];

    n1 = yf_node_init();
    snprintf(s, sizeof s, "n1 <%p>", (void *)n1);
    YF_TEST_PRINT("init", "", s);

    n2 = yf_node_init();
    snprintf(s, sizeof s, "n2 <%p>", (void *)n2);
    YF_TEST_PRINT("init", "", s);

    n3 = yf_node_init();
    snprintf(s, sizeof s, "n3 <%p>", (void *)n3);
    YF_TEST_PRINT("init", "", s);

    n4 = yf_node_init();
    snprintf(s, sizeof s, "n4 <%p>", (void *)n4);
    YF_TEST_PRINT("init", "", s);

    n5 = yf_node_init();
    snprintf(s, sizeof s, "n5 <%p>", (void *)n5);
    YF_TEST_PRINT("init", "", s);

    if (n1 == NULL || n2 == NULL || n3 == NULL || n4 == NULL || n5 == NULL)
        return -1;

    YF_TEST_PRINT("insert", "n1, n2", "");
    YF_TEST_PRINT("insert", "n1, n3", "");
    YF_TEST_PRINT("insert", "n1, n4", "");
    YF_TEST_PRINT("insert", "n3, n5", "");

    yf_node_insert(n1, n2);
    yf_node_insert(n1, n3);
    yf_node_insert(n1, n4);
    yf_node_insert(n3, n5);

    YF_TEST_PRINT("setname", "n1, \"node1\"", "");
    YF_TEST_PRINT("setname", "n2, \"node2\"", "");
    YF_TEST_PRINT("setname", "n3, \"node3\"", "");
    YF_TEST_PRINT("setname", "n4, \"node4\"", "");
    YF_TEST_PRINT("setname", "n5, \"node5\"", "");

    yf_node_setname(n1, "node1");
    yf_node_setname(n2, "node2");
    yf_node_setname(n3, "node3");
    yf_node_setname(n4, "node4");
    yf_node_setname(n5, "node5");

    YF_TEST_PRINT("traverse", "n1, do_each_node, 0", "");
    yf_node_traverse(n1, do_each_node, 0);

    YF_TEST_PRINT("traverse", "n2, do_each_node, 0", "");
    yf_node_traverse(n2, do_each_node, 0);

    YF_TEST_PRINT("traverse", "n3, do_each_node, 0", "");
    yf_node_traverse(n3, do_each_node, 0);

    YF_TEST_PRINT("traverse", "n4, do_each_node, 0", "");
    yf_node_traverse(n4, do_each_node, 0);

    YF_TEST_PRINT("traverse", "n5, do_each_node, 0", "");
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
    YF_TEST_PRINT("descends", "...", ""); \
    printf(" n2, n1: %s\n", chk[0] ? "yes" : "no"); \
    printf(" n3, n1: %s\n", chk[1] ? "yes" : "no"); \
    printf(" n4, n1: %s\n", chk[2] ? "yes" : "no"); \
    printf(" n5, n1: %s\n", chk[3] ? "yes" : "no"); \
    printf(" n5, n2: %s\n", chk[4] ? "yes" : "no"); \
    printf(" n5, n3: %s\n", chk[5] ? "yes" : "no"); \
    printf(" n1, n5: %s\n", chk[6] ? "yes" : "no"); } while (0)

#define YF_LEAFCHK() do { \
    chk[0] = yf_node_isleaf(n1); \
    chk[1] = yf_node_isleaf(n2); \
    chk[2] = yf_node_isleaf(n3); \
    chk[3] = yf_node_isleaf(n4); \
    chk[4] = yf_node_isleaf(n5); \
    \
    YF_TEST_PRINT("isleaf", "...", ""); \
    printf(" n1: %s\n", chk[0] ? "yes" : "no"); \
    printf(" n2: %s\n", chk[1] ? "yes" : "no"); \
    printf(" n3: %s\n", chk[2] ? "yes" : "no"); \
    printf(" n4: %s\n", chk[3] ? "yes" : "no"); \
    printf(" n5: %s\n", chk[4] ? "yes" : "no"); } while (0)

#define YF_ROOTCHK() do { \
    chk[0] = yf_node_isroot(n1); \
    chk[1] = yf_node_isroot(n2); \
    chk[2] = yf_node_isroot(n3); \
    chk[3] = yf_node_isroot(n4); \
    chk[4] = yf_node_isroot(n5); \
    \
    YF_TEST_PRINT("isroot", "...", ""); \
    printf(" n1: %s\n", chk[0] ? "yes" : "no"); \
    printf(" n2: %s\n", chk[1] ? "yes" : "no"); \
    printf(" n3: %s\n", chk[2] ? "yes" : "no"); \
    printf(" n4: %s\n", chk[3] ? "yes" : "no"); \
    printf(" n5: %s\n", chk[4] ? "yes" : "no"); } while (0)

#define YF_PNTCHK() do { \
    pnt[0] = yf_node_getparent(n1); \
    pnt[1] = yf_node_getparent(n2); \
    pnt[2] = yf_node_getparent(n3); \
    pnt[3] = yf_node_getparent(n4); \
    pnt[4] = yf_node_getparent(n5); \
    YF_TEST_PRINT("getparent", "...", ""); \
    printf(" n1: %p\n", (void *)pnt[0]); \
    printf(" n2: %p\n", (void *)pnt[1]); \
    printf(" n3: %p\n", (void *)pnt[2]); \
    printf(" n4: %p\n", (void *)pnt[3]); \
    printf(" n5: %p\n", (void *)pnt[4]); } while (0)

#define YF_LENCHK() do { \
    len[0] = yf_node_getlen(n1); \
    len[1] = yf_node_getlen(n2); \
    len[2] = yf_node_getlen(n3); \
    len[3] = yf_node_getlen(n4); \
    len[4] = yf_node_getlen(n5); \
    \
    YF_TEST_PRINT("getlen", "...", ""); \
    printf(" n1: %zu\n", len[0]); \
    printf(" n2: %zu\n", len[1]); \
    printf(" n3: %zu\n", len[2]); \
    printf(" n4: %zu\n", len[3]); \
    printf(" n5: %zu\n", len[4]); } while (0)

    YF_DESCCHK();
    if (!chk[0] || !chk[1] || !chk[2] || !chk[3] || chk[4] || !chk[5] || chk[6])
        return -1;
    YF_LEAFCHK();
    if (chk[0] || !chk[1] || chk[2] || !chk[3] || !chk[4])
        return -1;
    YF_ROOTCHK();
    if (!chk[0] || chk[1] || chk[2] || chk[3] || chk[4])
        return -1;
    YF_PNTCHK();
    if (pnt[0] != NULL || pnt[1] != n1 || pnt[2] != n1 || pnt[3] != n1 ||
        pnt[4] != n3)
        return -1;
    YF_LENCHK();
    if (len[0] != 5 || len[1] != 1 || len[2] != 2 || len[3] != 1 || len[4] != 1)
        return -1;

    YF_TEST_PRINT("cmpname", "n2, \"node2\"", "");
    if (yf_node_cmpname(n2, "node2") != 0)
        return -1;
    yf_node_setname(n2, "NODE2");
    if (yf_node_cmpname(n2, "node2") == 0)
        return -1;

    YF_TEST_PRINT("cmpname", "n5, \"node-5\"", "");
    if (yf_node_cmpname(n5, "node-5") == 0)
        return -1;
    yf_node_setname(n5, "node-5");
    if (yf_node_cmpname(n5, "node-5") != 0)
        return -1;

    YF_TEST_PRINT("drop", "n3", "");
    yf_node_drop(n3);

    YF_TEST_PRINT("traverse", "n1, do_each_node, 0", "");
    yf_node_traverse(n1, do_each_node, 0);

    YF_TEST_PRINT("traverse", "n3, do_each_node, 0", "");
    yf_node_traverse(n3, do_each_node, 0);

    YF_DESCCHK();
    if (!chk[0] || chk[1] || !chk[2] || chk[3] || chk[4] || !chk[5] || chk[6])
        return -1;
    YF_LEAFCHK();
    if (chk[0] || !chk[1] || chk[2] || !chk[3] || !chk[4])
        return -1;
    YF_ROOTCHK();
    if (!chk[0] || chk[1] || !chk[2] || chk[3] || chk[4])
        return -1;
    YF_PNTCHK();
    if (pnt[0] != NULL || pnt[1] != n1 || pnt[2] != NULL || pnt[3] != n1 ||
        pnt[4] != n3)
        return -1;
    YF_LENCHK();
    if (len[0] != 3 || len[1] != 1 || len[2] != 2 || len[3] != 1 || len[4] != 1)
        return -1;

    YF_TEST_PRINT("prune", "n1", "");
    yf_node_prune(n1);

    YF_TEST_PRINT("traverse", "n1, do_each_node, 0", "");
    yf_node_traverse(n1, do_each_node, 0);

    YF_TEST_PRINT("traverse", "n3, do_each_node, 0", "");
    yf_node_traverse(n3, do_each_node, 0);

    YF_DESCCHK();
    if (chk[0] || chk[1] || chk[2] || chk[3] || chk[4] || !chk[5] || chk[6])
        return -1;
    YF_LEAFCHK();
    if (!chk[0] || !chk[1] || chk[2] || !chk[3] || !chk[4])
        return -1;
    YF_ROOTCHK();
    if (!chk[0] || !chk[1] || !chk[2] || !chk[3] || chk[4])
        return -1;
    YF_PNTCHK();
    if (pnt[0] != NULL || pnt[1] != NULL || pnt[2] != NULL || pnt[3] != NULL ||
        pnt[4] != n3)
        return -1;
    YF_LENCHK();
    if (len[0] != 1 || len[1] != 1 || len[2] != 2 || len[3] != 1 || len[4] != 1)
        return -1;

    YF_TEST_PRINT("deinit", "n1", "");
    YF_TEST_PRINT("deinit", "n2", "");
    YF_TEST_PRINT("deinit", "n3", "");
    YF_TEST_PRINT("deinit", "n4", "");
    YF_TEST_PRINT("deinit", "n5", "");

    yf_node_deinit(n1);
    yf_node_deinit(n2);
    yf_node_deinit(n3);
    yf_node_deinit(n4);
    yf_node_deinit(n5);

    return 0;
}
