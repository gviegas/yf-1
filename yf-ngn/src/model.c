/*
 * YF
 * model.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "yf-model.h"
#include "node.h"

struct yf_model {
    yf_node_t *node;
    yf_mesh_t *mesh;
    yf_skin_t *skin;
    yf_skeleton_t *skel;
};

/* Model deinitialization callback. */
static void deinit_mdl(void *mdl)
{
    free(mdl);
}

yf_model_t *yf_model_init(void)
{
    yf_model_t *mdl = calloc(1, sizeof(yf_model_t));
    if (mdl == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    if ((mdl->node = yf_node_init()) == NULL) {
        yf_model_deinit(mdl);
        return NULL;
    }

    yf_node_setobj(mdl->node, YF_NODEOBJ_MODEL, mdl, deinit_mdl);
    return mdl;
}

yf_node_t *yf_model_getnode(yf_model_t *mdl)
{
    assert(mdl != NULL);
    return mdl->node;
}

yf_mesh_t *yf_model_getmesh(yf_model_t *mdl)
{
    assert(mdl != NULL);
    return mdl->mesh;
}

void yf_model_setmesh(yf_model_t *mdl, yf_mesh_t *mesh)
{
    assert(mdl != NULL);
    mdl->mesh = mesh;
}

yf_skin_t *yf_model_getskin(yf_model_t *mdl, yf_skeleton_t **skel)
{
    assert(mdl != NULL);
    assert(skel != NULL);

    *skel = mdl->skel;
    return mdl->skin;
}

void yf_model_setskin(yf_model_t *mdl, yf_skin_t *skin, yf_skeleton_t *skel)
{
    assert(mdl != NULL);

    mdl->skin = skin;
    mdl->skel = skel;
}

void yf_model_deinit(yf_model_t *mdl)
{
    if (mdl != NULL)
        yf_node_deinit(mdl->node);
}
