/*
 * YF
 * terrain.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "yf-terrain.h"
#include "node.h"
#include "mesh.h"
#include "vertex.h"

struct YF_terrain_o {
    YF_node node;
    unsigned width;
    unsigned depth;
    YF_mesh mesh;
    YF_texture hmap;
    YF_texture tex;
    /* TODO: Other terrain properties. */
};

/* Initializes grid mesh. */
static int init_grid(YF_terrain terr)
{
    assert(terr != NULL);

    /* TODO: Support for custom tiling. */

    const unsigned wdt = terr->width;
    const unsigned dep = terr->depth;

    YF_meshdt data = {0};
    data.v.vtype = YF_VTYPE_TERR;
    data.v.n = (wdt+1) * (dep+1);
    data.i.n = wdt * dep * 6;
    if (data.v.n < 65535)
        data.i.stride = sizeof(unsigned short);
    else
        data.i.stride = sizeof(unsigned);

    data.v.data = malloc(data.v.n * sizeof(YF_vterr));
    data.i.data = malloc(data.i.n * data.i.stride);
    if (data.v.data == NULL || data.i.data == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(data.v.data);
        free(data.i.data);
        return -1;
    }

#define YF_GRID_CPYI() do { \
    unsigned idx = 0; \
    unsigned k; \
    for (unsigned i = 0; i < wdt; i++) { \
        for (unsigned j = 0; j < dep; j++) { \
            k = (dep+1) * i + j; \
            inds[idx++] = k; \
            inds[idx++] = k+1; \
            inds[idx++] = k+dep+2; \
            inds[idx++] = k; \
            inds[idx++] = k+dep+2; \
            inds[idx++] = k+dep+1; \
        } \
    } } while (0)

    if (data.i.stride != sizeof(unsigned)) {
        unsigned short *inds = data.i.data;
        YF_GRID_CPYI();
    } else {
        unsigned *inds = data.i.data;
        YF_GRID_CPYI();
    }

#undef YF_GRID_CPYI

    float x0, z0;
    float pos_off;
    if (wdt > dep) {
        x0 = -1.0f;
        z0 = -(float)dep / (float)wdt;
        pos_off = 2.0f / (float)wdt;
    } else {
        x0 = -(float)wdt / (float)dep;
        z0 = -1.0f;
        pos_off = 2.0f / (float)dep;
    }
    /* NxN textures are expected even for non-square grids */
    float tc_off = pos_off / 2.0f;

    float x, z, s, t;
    unsigned k;
    YF_vterr *vdt = data.v.data;
    for (unsigned i = 0; i <= wdt; i++) {
        x = x0 + pos_off * (float)i;
        s = tc_off * (float)i;
        for (unsigned j = 0; j <= dep; j++) {
            z = z0 + pos_off * (float)j;
#ifdef YF_FLIP_TEX
            t = tc_off * (float)(dep-j);
#else
            t = 1.0f - tc_off * (float)(dep-j);
#endif
            k = (dep+1) * i + j;
            vdt[k].pos[0] = x;
            vdt[k].pos[1] = 0.0f;
            vdt[k].pos[2] = z;
            vdt[k].tc[0] = s;
            vdt[k].tc[1] = t;
            vdt[k].norm[0] = 0.0f;
            vdt[k].norm[1] = 1.0f;
            vdt[k].norm[2] = 0.0f;
        }
    }

    terr->mesh = yf_mesh_initdt(&data);
    free(data.v.data);
    free(data.i.data);
    if (terr->mesh == NULL)
        return -1;
    return 0;
}

YF_terrain yf_terrain_init(unsigned width, unsigned depth)
{
    if (width == 0 || depth == 0) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    YF_terrain terr = calloc(1, sizeof(struct YF_terrain_o));
    if (terr == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    if ((terr->node = yf_node_init()) == NULL) {
        free(terr);
        return NULL;
    }
    yf_node_setobj(terr->node, YF_NODEOBJ_TERRAIN, terr);
    terr->width = width;
    terr->depth = depth;

    if (init_grid(terr) != 0) {
        yf_terrain_deinit(terr);
        return NULL;
    }
    return terr;
}

YF_node yf_terrain_getnode(YF_terrain terr)
{
    assert(terr != NULL);
    return terr->node;
}

YF_mesh yf_terrain_getmesh(YF_terrain terr)
{
    assert(terr != NULL);
    return terr->mesh;
}

YF_texture yf_terrain_gethmap(YF_terrain terr)
{
    assert(terr != NULL);
    return terr->hmap;
}

void yf_terrain_sethmap(YF_terrain terr, YF_texture hmap)
{
    assert(terr != NULL);
    terr->hmap = hmap;
}

YF_texture yf_terrain_gettex(YF_terrain terr)
{
    assert(terr != NULL);
    return terr->tex;
}

void yf_terrain_settex(YF_terrain terr, YF_texture tex)
{
    assert(terr != NULL);
    terr->tex = tex;
}

void yf_terrain_deinit(YF_terrain terr)
{
    if (terr != NULL) {
        yf_node_deinit(terr->node);
        yf_mesh_deinit(terr->mesh);
        free(terr);
    }
}
