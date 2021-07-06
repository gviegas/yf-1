/*
 * YF
 * skin.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "yf-skin.h"
#include "yf-node.h"

struct YF_skin_o {
    YF_joint *jnts;
    unsigned jnt_n;
};

struct YF_skeleton_o {
    YF_node *nodes;
    unsigned node_n;
};

YF_skin yf_skin_init(const YF_joint *jnts, unsigned jnt_n)
{
    if (jnts == NULL || jnt_n == 0) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    YF_skin skin = malloc(sizeof(struct YF_skin_o));
    if (skin == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    skin->jnts = malloc(jnt_n * sizeof *jnts);
    if (skin->jnts == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(skin);
        return NULL;
    }
    memcpy(skin->jnts, jnts, jnt_n * sizeof *jnts);
    skin->jnt_n = jnt_n;

    return skin;
}

YF_skeleton yf_skin_makeskel(YF_skin skin)
{
    assert(skin != NULL);

    YF_skeleton skel = malloc(sizeof(struct YF_skeleton_o));
    if (skel == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    skel->node_n = skin->jnt_n + 1;
    skel->nodes = malloc(skel->node_n * sizeof *skel->nodes);
    if (skel->nodes == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(skel);
        return NULL;
    }

    for (unsigned i = 0; i < skel->node_n; i++) {
        if ((skel->nodes[i] = yf_node_init()) == NULL) {
            for (unsigned j = 0; j < i; j++)
                yf_node_deinit(skel->nodes[j]);
            free(skel->nodes);
            free(skel);
            return NULL;
        }
    }

    for (unsigned i = 0; i < skin->jnt_n; i++) {
        YF_node node = skel->nodes[i];
        YF_joint *jnt = skin->jnts+i;

        yf_mat4_copy(*yf_node_getxform(node), jnt->xform);
        /* XXX: Make sure name is null-terminated (on init()). */
        yf_node_setname(node, jnt->name);

        /* XXX: Make sure index is less than joint count (on init()). */
        if (jnt->pnt_i < 0)
            yf_node_insert(skel->nodes[skin->jnt_n], node);
        else
            yf_node_insert(skel->nodes[jnt->pnt_i], node);
    }

    assert(!yf_node_isleaf(skel->nodes[skin->jnt_n]));

    return skel;
}

void yf_skin_unmkskel(YF_skin skin, YF_skeleton skel)
{
    assert(skin != NULL);

    if (skel == NULL)
        return;

    for (unsigned i = 0; i < skel->node_n; i++)
        yf_node_deinit(skel->nodes[i]);
    free(skel->nodes);
    free(skel);
}

void yf_skin_deinit(YF_skin skin)
{
    if (skin != NULL) {
        free(skin->jnts);
        free(skin);
    }
}
