/*
 * YF
 * test-skin.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"
#include "print.h"
#include "yf-skin.h"

/* Tests skin. */
int yf_test_skin(void)
{
    YF_node node = yf_node_init();
    assert(node != NULL);

    YF_joint jnts[] = {
        {{0}, {0}, "joint-a", -1},
        {{0}, {0}, "joint-b", 0},
        {{0}, {0}, "joint-c", -1},
        {{0}, {0}, "joint-d", 4},
        {{0}, {0}, "joint-e", 2}
    };

    yf_mat4_xlate(jnts[0].xform, 1.0f, 2.0f, 3.0f);
    yf_mat4_xlate(jnts[0].ibm, 4.0f, 5.0f, 6.0f);
    yf_mat4_xlate(jnts[1].xform, 7.0f, 8.0f, 9.0f);
    yf_mat4_rotx(jnts[1].ibm, 3.141593f * 0.25f);
    yf_mat4_roty(jnts[2].xform, 3.141593f * 0.25f);
    yf_mat4_rotz(jnts[2].ibm, 3.141593f * 0.25f);
    yf_mat4_scale(jnts[3].xform, 0.1f, 0.2f, 0.3f);
    yf_mat4_scale(jnts[3].ibm, 0.4f, 0.5f, 0.6f);
    yf_mat4_scale(jnts[4].xform, 0.7f, 0.8f, 0.9f);
    yf_mat4_inv(jnts[4].ibm, jnts[4].xform);

    YF_TEST_PRINT("init", "jnts, 5", "skin");
    YF_skin skin = yf_skin_init(jnts, 5);
    if (skin == NULL)
        return -1;

    yf_print_skin(skin);

    YF_TEST_PRINT("newest", "skin", "");
    if (yf_skin_newest(skin) != NULL)
        return -1;

    YF_TEST_PRINT("makeskel", "skin, NULL", "skel");
    YF_skeleton skel = yf_skin_makeskel(skin, NULL);
    if (skel == NULL)
        return -1;

    yf_print_skin(skin);

    YF_TEST_PRINT("newest", "skin", "");
    if (yf_skin_newest(skin) != skel)
        return -1;

    YF_TEST_PRINT("makeskel", "skin, NULL", "skel2");
    YF_skeleton skel2 = yf_skin_makeskel(skin, NULL);
    if (skel2 == NULL)
        return -1;

    yf_print_skin(skin);

    YF_TEST_PRINT("newest", "skin", "");
    if (yf_skin_newest(skin) != skel2)
        return -1;

    YF_TEST_PRINT("unmkskel", "skin, skel2", "");
    yf_skin_unmkskel(skin, skel2);

    yf_print_skin(skin);

    YF_TEST_PRINT("newest", "skin", "");
    if (yf_skin_newest(skin) != skel)
        return -1;

    YF_TEST_PRINT("unmkskel", "skin, skel", "");
    yf_skin_unmkskel(skin, skel);

    yf_print_skin(skin);

    YF_TEST_PRINT("newest", "skin", "");
    if (yf_skin_newest(skin) != NULL)
        return -1;

    YF_TEST_PRINT("deinit", "skin", "");
    yf_skin_deinit(skin);

    YF_TEST_PRINT("init", "jnts, 1", "skin");
    skin = yf_skin_init(jnts, 1);
    if (skin == NULL)
        return -1;

    yf_print_skin(skin);

    YF_TEST_PRINT("init", "jnts, 2", "skin2");
    YF_skin skin2 = yf_skin_init(jnts, 2);
    if (skin2 == NULL)
        return -1;

    yf_print_skin(skin2);

    YF_node nodes[] = {node, node};

    YF_TEST_PRINT("makeskel", "skin, nodes", "skel");
    skel = yf_skin_makeskel(skin, nodes);
    if (skel == NULL)
        return -1;

    yf_print_skin(skin);

    YF_TEST_PRINT("getnode", "skin, skel", "");
    if (yf_skin_getnode(skin, skel) != node)
        return -1;

    YF_TEST_PRINT("getjntnode", "skin, skel, 0", "");
    if (yf_skin_getjntnode(skin, skel, 0) != node)
        return -1;

    YF_TEST_PRINT("makeskel", "skin2, NULL", "skel2");
    skel2 = yf_skin_makeskel(skin2, NULL);
    if (skel2 == NULL)
        return -1;

    yf_print_skin(skin2);

    YF_TEST_PRINT("getnode", "skin2, skel2", "");
    YF_node node2 = yf_skin_getnode(skin2, skel2);
    if (node2 == NULL)
        return -1;

    YF_TEST_PRINT("getjntnode", "skin2, skel2, 0", "");
    YF_node node3 = yf_skin_getjntnode(skin2, skel2, 0);
    if (node3 == NULL || node3 == node2)
        return -1;

    YF_TEST_PRINT("getjntnode", "skin2, skel2, 1", "");
    YF_node node4 = yf_skin_getjntnode(skin2, skel2, 1);
    if (node4 == NULL || node4 == node3 || node4 == node2)
        return -1;

    YF_TEST_PRINT("deinit", "skin", "");
    yf_skin_deinit(skin);

    YF_TEST_PRINT("deinit", "skin2", "");
    yf_skin_deinit(skin2);

    yf_node_deinit(node);

    return 0;
}
