/*
 * YF
 * skin.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef YF_DEVEL
# include <stdio.h>
#endif

#include "yf/com/yf-list.h"
#include "yf/com/yf-error.h"

#include "yf-skin.h"

struct YF_skin_o {
    YF_joint *jnts;
    unsigned jnt_n;
    YF_list skels;
};

struct YF_skeleton_o {
    YF_node *nodes;
    unsigned node_n;
    int managed;
};

YF_skin yf_skin_init(const YF_joint *jnts, unsigned jnt_n)
{
    if (jnts == NULL || jnt_n == 0) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    for (unsigned i = 0; i < jnt_n; i++) {
        if (jnts[i].pnt_i >= jnt_n || memchr(jnts[i].name, '\0',
                                             sizeof jnts[i].name) == NULL) {
            yf_seterr(YF_ERR_INVARG, __func__);
            return NULL;
        }
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

    skin->skels = yf_list_init(NULL);
    if (skin->skels == NULL) {
        free(skin->jnts);
        free(skin);
        return NULL;
    }

    return skin;
}

const YF_joint *yf_skin_getjnts(YF_skin skin, unsigned *jnt_n)
{
    assert(skin != NULL);
    assert(jnt_n != NULL);

    *jnt_n = skin->jnt_n;
    return skin->jnts;
}

void yf_skin_each(YF_skin skin, int (*callb)(YF_skeleton skel, void *arg),
                  void *arg)
{
    assert(skin != NULL);
    assert(callb != NULL);

    YF_iter it = YF_NILIT;
    YF_skeleton skel;
    while ((skel = yf_list_next(skin->skels, &it)) != NULL &&
           callb(skel, arg) == 0)
        ;
}

YF_skeleton yf_skin_newest(YF_skin skin)
{
    assert(skin != NULL);
    return yf_list_next(skin->skels, NULL);
}

YF_skeleton yf_skin_makeskel(YF_skin skin, const YF_node *nodes)
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

    if (nodes == NULL) {
        /* nodes are managed by the skeleton */
        skel->managed = 1;

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

            if (yf_node_setname(node, jnt->name) != 0) {
                yf_skin_unmkskel(skin, skel);
                return NULL;
            }

            if (jnt->pnt_i < 0)
                yf_node_insert(skel->nodes[skin->jnt_n], node);
            else
                yf_node_insert(skel->nodes[jnt->pnt_i], node);
        }

        /* XXX */
        assert(!yf_node_isleaf(skel->nodes[skin->jnt_n]));

    } else {
        /* nodes are externally managed */
        skel->managed = 0;
        memcpy(skel->nodes, nodes, skel->node_n * sizeof *skel->nodes);
    }

    if (yf_list_insert(skin->skels, skel) != 0) {
        yf_skin_unmkskel(skin, skel);
        return NULL;
    }
    return skel;
}

YF_node yf_skin_getnode(YF_skin skin, YF_skeleton skel)
{
    assert(skin != NULL);
    assert(skel != NULL);

    return skel->nodes[skin->jnt_n];
}

YF_node yf_skin_getjntnode(YF_skin skin, YF_skeleton skel, unsigned index)
{
    assert(skin != NULL);
    assert(skel != NULL);
    assert(index < skin->jnt_n);

    return skel->nodes[index];
}

void yf_skin_unmkskel(YF_skin skin, YF_skeleton skel)
{
    assert(skin != NULL);

    if (skel == NULL)
        return;

    /* XXX: This call will fail when 'makeskel()' itself fails. */
    yf_list_remove(skin->skels, skel);

    if (skel->managed) {
        for (unsigned i = 0; i < skel->node_n; i++)
            yf_node_deinit(skel->nodes[i]);
    }
    free(skel->nodes);
    free(skel);
}

void yf_skin_deinit(YF_skin skin)
{
    if (skin == NULL)
        return;

    YF_skeleton skel;
    while ((skel = yf_list_removeat(skin->skels, NULL)) != NULL)
        yf_skin_unmkskel(skin, skel);

    yf_list_deinit(skin->skels);
    free(skin->jnts);
    free(skin);
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

void yf_print_skin(YF_skin skin)
{
    assert(skin != NULL);

    printf("\n[YF] OUTPUT (%s):\n"
           " skin:\n"
           " joints (%u):\n",
           __func__, skin->jnt_n);

    for (unsigned i = 0; i < skin->jnt_n; i++) {
        const YF_joint *jnt = skin->jnts+i;
        printf("  joint [%u]:\n"
               "   transform:\n"
               "    %.4f  %.4f  %.4f  %.4f\n"
               "    %.4f  %.4f  %.4f  %.4f\n"
               "    %.4f  %.4f  %.4f  %.4f\n"
               "    %.4f  %.4f  %.4f  %.4f\n"
               "   inverse-bind matrix:\n"
               "    %.4f  %.4f  %.4f  %.4f\n"
               "    %.4f  %.4f  %.4f  %.4f\n"
               "    %.4f  %.4f  %.4f  %.4f\n"
               "    %.4f  %.4f  %.4f  %.4f\n"
               "   name: '%s'\n"
               "   parent index: %ld%s\n",
               i,
               jnt->xform[0], jnt->xform[4], jnt->xform[8], jnt->xform[12],
               jnt->xform[1], jnt->xform[5], jnt->xform[9], jnt->xform[13],
               jnt->xform[2], jnt->xform[6], jnt->xform[10], jnt->xform[14],
               jnt->xform[3], jnt->xform[7], jnt->xform[11], jnt->xform[15],
               jnt->ibm[0], jnt->ibm[4], jnt->ibm[8], jnt->ibm[12],
               jnt->ibm[1], jnt->ibm[5], jnt->ibm[9], jnt->ibm[13],
               jnt->ibm[2], jnt->ibm[6], jnt->ibm[10], jnt->ibm[14],
               jnt->ibm[3], jnt->ibm[7], jnt->ibm[11], jnt->ibm[15],
               jnt->name, jnt->pnt_i, (jnt->pnt_i < 0 ? " (no parent)" : ""));
    }

    const size_t skel_n = yf_list_getlen(skin->skels);
    if (skel_n != 0) {
        printf("  skeletons (%zu):\n", skel_n);

        YF_iter it = YF_NILIT;
        YF_skeleton skel;
        while ((skel = yf_list_next(skin->skels, &it)) != NULL) {
            printf("   skeleton <%p>:\n"
                   "    managed: %s\n"
                   "    nodes: (%u):\n",
                   (void *)skel, (skel->managed ? "yes" : "no"), skel->node_n);

            for (unsigned i = 0; i < skel->node_n; i++) {
                size_t len;
                yf_node_getname(skel->nodes[i], NULL, &len);
                char str[len];
                yf_node_getname(skel->nodes[i], str, &len);

                printf("     [%.3u]: '%s' <%p> ",
                       i, str, (void *)skel->nodes[i]);

                if (yf_node_isroot(skel->nodes[i]))
                    puts("(is a root node)");
                else
                    printf("(parent is <%p>)\n",
                           (void *)yf_node_getparent(skel->nodes[i]));
            }
        }
    } else {
        puts("  (no skeletons instantiated)\n");
    }

    puts("");
}

#endif
