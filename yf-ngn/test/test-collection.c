/*
 * YF
 * test-collection.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "yf/com/yf-error.h"

#include "test.h"
#include "print.h"
#include "yf-ngn.h"

/* Tests collection. */
int yf_test_collection(void)
{
    struct { char *name; YF_node node; } nodes[] = {
        {"node1", yf_node_init()},
        {"node2", yf_node_init()},
        {"node3", yf_node_init()},
        {"node4", yf_node_init()}
    };

    struct { char *name; YF_material matl; } matls[] = {
        {"matl1", yf_material_init(NULL)},
        {"matl2", yf_material_init(NULL)}
    };

    char s[256] = {0};

    YF_TEST_PRINT("init", "NULL", "coll");
    YF_collection coll = yf_collection_init(NULL);
    if (coll == NULL)
        return -1;

    yf_print_coll(coll);

    for (size_t i = 0; i < (sizeof nodes / sizeof *nodes); i++) {
        snprintf(s, sizeof s, "coll, CITEM_NODE, \"%s\", <%p>", nodes[i].name,
                 (void *)nodes[i].node);
        YF_TEST_PRINT("manage", s, "");
        if (yf_collection_manage(coll, YF_CITEM_NODE, nodes[i].name,
                                 nodes[i].node) != 0)
            return -1;

        snprintf(s, sizeof s, "coll, CITEM_NODE, \"%s\"", nodes[i].name);
        YF_TEST_PRINT("contains", s, "");
        if (!yf_collection_contains(coll, YF_CITEM_NODE, nodes[i].name))
            return -1;
    }

    yf_print_coll(coll);

    for (size_t i = 0; i < (sizeof matls / sizeof *matls); i++) {
        snprintf(s, sizeof s, "coll, CITEM_MATERIAL, \"%s\", <%p>",
                 matls[i].name, (void *)matls[i].matl);
        YF_TEST_PRINT("manage", s, "");
        if (yf_collection_manage(coll, YF_CITEM_MATERIAL, matls[i].name,
                                 matls[i].matl) != 0)
            return -1;

        snprintf(s, sizeof s, "coll, CITEM_MATERIAL, \"%s\"", matls[i].name);
        YF_TEST_PRINT("contains", s, "");
        if (!yf_collection_contains(coll, YF_CITEM_MATERIAL, matls[i].name))
            return -1;
    }

    yf_print_coll(coll);

    snprintf(s, sizeof s, "coll, CITEM_NODE, \"%s\"", nodes[1].name);
    YF_TEST_PRINT("getitem", s, "");
    if (yf_collection_getitem(coll, YF_CITEM_NODE,
                              nodes[1].name) != nodes[1].node)
        return -1;

    snprintf(s, sizeof s, "coll, CITEM_NODE, \"%s\"", nodes[0].name);
    YF_TEST_PRINT("getitem", s, "");
    if (yf_collection_getitem(coll, YF_CITEM_NODE,
                              nodes[0].name) != nodes[0].node)
        return -1;

    snprintf(s, sizeof s, "coll, CITEM_MATERIAL, \"%s\"", matls[1].name);
    YF_TEST_PRINT("getitem", s, "");
    if (yf_collection_getitem(coll, YF_CITEM_MATERIAL,
                              matls[1].name) != matls[1].matl)
        return -1;

    snprintf(s, sizeof s, "coll, CITEM_MATERIAL, \"%s\"", matls[0].name);
    YF_TEST_PRINT("getitem", s, "");
    if (yf_collection_getitem(coll, YF_CITEM_MATERIAL,
                              matls[0].name) != matls[0].matl)
        return -1;

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    snprintf(s, sizeof s, "coll, CITEM_NODE, \"%s\"", matls[1].name);
    YF_TEST_PRINT("getitem", s, "");
    if (yf_collection_getitem(coll, YF_CITEM_NODE, matls[1].name) != NULL ||
        yf_geterr() != YF_ERR_NOTFND)
        return -1;
    yf_printerr();

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    snprintf(s, sizeof s, "coll, CITEM_MATERIAL, \"%s\"", nodes[0].name);
    YF_TEST_PRINT("getitem", s, "");
    if (yf_collection_getitem(coll, YF_CITEM_MATERIAL, nodes[0].name) != NULL ||
        yf_geterr() != YF_ERR_NOTFND)
        return -1;
    yf_printerr();

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    YF_TEST_PRINT("getitem", "coll, YF_CITEM_SCENE, \"scene1\"", "");
    if (yf_collection_getitem(coll, YF_CITEM_SCENE, "scene1") != NULL ||
        yf_geterr() != YF_ERR_NOTFND)
        return -1;
    yf_printerr();

    for (size_t i = 0; i < (sizeof nodes / sizeof *nodes); i++) {
        snprintf(s, sizeof s, "coll, CITEM_NODE, \"%s\"", nodes[i].name);
        YF_TEST_PRINT("release", s, "");
        if (yf_collection_release(coll, YF_CITEM_NODE,
                                  nodes[i].name) != nodes[i].node)
            return -1;

        YF_TEST_PRINT("contains", s, "");
        if (yf_collection_contains(coll, YF_CITEM_NODE, nodes[i].name))
            return -1;
    }

    yf_print_coll(coll);

    for (size_t i = 0; i < (sizeof matls / sizeof *matls); i++) {
        snprintf(s, sizeof s, "coll, CITEM_MATERIAL, \"%s\"", matls[i].name);
        YF_TEST_PRINT("release", s, "");
        if (yf_collection_release(coll, YF_CITEM_MATERIAL,
                                  matls[i].name) != matls[i].matl)
            return -1;

        YF_TEST_PRINT("contains", s, "");
        if (yf_collection_contains(coll, YF_CITEM_MATERIAL, matls[i].name))
            return -1;
    }

    yf_print_coll(coll);

    YF_TEST_PRINT("init", "tmp/scene.glb", "coll2");
    YF_collection coll2 = yf_collection_init("tmp/scene.glb");
    if (coll2 == NULL)
        return -1;

    yf_print_coll(coll2);

    YF_TEST_PRINT("deinit", "coll", "");
    yf_collection_deinit(coll);

    YF_TEST_PRINT("deinit", "coll2", "");
    yf_collection_deinit(coll2);

    for (size_t i = 0; i < (sizeof nodes / sizeof *nodes); i++)
        yf_node_deinit(nodes[i].node);
    for (size_t i = 0; i < (sizeof matls / sizeof *matls); i++)
        yf_material_deinit(matls[i].matl);

    return 0;
}
