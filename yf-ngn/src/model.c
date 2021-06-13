/*
 * YF
 * model.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "yf-model.h"
#include "node.h"

struct YF_model_o {
    YF_node node;
    YF_mesh mesh;
    YF_material matl;
    /* TODO: Other model properties. */
};

/* Model deinitialization callback. */
static void deinit_mdl(void *mdl)
{
    free(mdl);
}

YF_model yf_model_init(void)
{
    YF_model mdl = calloc(1, sizeof(struct YF_model_o));
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

YF_node yf_model_getnode(YF_model mdl)
{
    assert(mdl != NULL);
    return mdl->node;
}

YF_mesh yf_model_getmesh(YF_model mdl)
{
    assert(mdl != NULL);
    return mdl->mesh;
}

void yf_model_setmesh(YF_model mdl, YF_mesh mesh)
{
    assert(mdl != NULL);
    mdl->mesh = mesh;
}

YF_material yf_model_getmatl(YF_model mdl)
{
    assert(mdl != NULL);
    return mdl->matl;
}

void yf_model_setmatl(YF_model mdl, YF_material matl)
{
    assert(mdl != NULL);
    mdl->matl = matl;
}

void yf_model_deinit(YF_model mdl)
{
    if (mdl != NULL)
        yf_node_deinit(mdl->node);
}
