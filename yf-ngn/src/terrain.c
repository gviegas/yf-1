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
#include "resmgr.h"

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
    const unsigned vert_n = (wdt + 1) * (dep + 1);
    const unsigned indx_n = wdt * dep * 6;
    const size_t pos_sz = sizeof(float[3]) * vert_n;
    const size_t tc_sz = sizeof(float[2]) * vert_n;
    const size_t norm_sz = sizeof(float[3]) * vert_n;

    int itype;
    size_t indx_sz;
    if (vert_n < 65536) {
        itype = YF_ITYPE_USHORT;
        indx_sz = indx_n << 1;
    } else {
        itype = YF_ITYPE_UINT;
        indx_sz = indx_n << 2;
    }

    const size_t data_sz = pos_sz + tc_sz + norm_sz + indx_sz;
    void *dt = malloc(data_sz);
    if (dt == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    const YF_meshdt data = {
        .prims = &(YF_primdt){
            .primitive = YF_PRIMITIVE_TRIANGLE,
            .vert_n = vert_n,
            .indx_n = indx_n,
            .data_off = 0,
            .attrs = (YF_attrdt[]){
                [0] = {YF_RESLOC_POS, YF_VFMT_FLOAT3, 0},
                [1] = {YF_RESLOC_TC, YF_VFMT_FLOAT2, pos_sz},
                [2] = {YF_RESLOC_NORM, YF_VFMT_FLOAT3, pos_sz + tc_sz}
            },
            .attr_n = 3,
            .itype = itype,
            .indx_data_off = pos_sz + tc_sz + norm_sz
        },
        .prim_n = 1,
        .data = dt,
        .data_sz = data_sz
    };

#define YF_GRID_CPYI() do { \
    inds = (void *)( (char *)dt + data.prims->indx_data_off ); \
    for (unsigned i = 0; i < wdt; i++) { \
        for (unsigned j = 0; j < dep; j++) { \
            unsigned k = (dep + 1) * i + j; \
            *inds++ = k; \
            *inds++ = k + 1; \
            *inds++ = k + dep + 2; \
            *inds++ = k; \
            *inds++ = k + dep + 2; \
            *inds++ = k + dep + 1; \
        } \
    } } while (0)

    if (itype == YF_ITYPE_USHORT) {
        unsigned short *inds;
        YF_GRID_CPYI();
    } else {
        unsigned *inds;
        YF_GRID_CPYI();
    }
#undef YF_GRID_CPYI

    float x0, z0, pos_off;
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

    float *pos_dt = dt;
    float *tc_dt = (float *)( (char *)pos_dt + pos_sz );
    float *norm_dt = (float *)( (char *)tc_dt + tc_sz );

    for (unsigned i = 0; i <= wdt; i++) {
        float x = x0 + pos_off * (float)i;
        float s = tc_off * (float)i;

        for (unsigned j = 0; j <= dep; j++) {
            float z = z0 + pos_off * (float)j;
#ifdef YF_FLIP_TEX
            float t = tc_off * (float)(dep - j);
#else
            float t = 1.0f - tc_off * (float)(dep - j);
#endif

            *pos_dt++ = x;
            *pos_dt++ = 0.0f;
            *pos_dt++ = z;

            *tc_dt++ = s;
            *tc_dt++ = t;

            *norm_dt++ = 0.0f;
            *norm_dt++ = 1.0f;
            *norm_dt++ = 0.0f;
        }
    }

    terr->mesh = yf_mesh_initdt(&data);
    free(dt);
    return terr->mesh == NULL ? -1 : 0;
}

/* Terrain deinitialization callback. */
static void deinit_terr(void *terr)
{
    yf_mesh_deinit(((YF_terrain)terr)->mesh);
    free(terr);
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
    yf_node_setobj(terr->node, YF_NODEOBJ_TERRAIN, terr, deinit_terr);
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
    if (terr != NULL)
        yf_node_deinit(terr->node);
}
