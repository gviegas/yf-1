/*
 * YF
 * quad.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "yf-quad.h"
#include "node.h"
#include "mesh.h"

#define YF_VQUAD_POSN (3 << 2)
#define YF_VQUAD_TCN  (2 << 2)
#define YF_VQUAD_CLRN (4 << 2)
#define YF_VQUAD_N    (YF_VQUAD_POSN + YF_VQUAD_TCN + YF_VQUAD_CLRN)

struct yf_quad {
    yf_node_t *node;
    float verts[YF_VQUAD_N];
    yf_mesh_t *mesh;
    yf_texture_t *tex;
    yf_rect_t rect;
#define YF_PEND_NONE 0
#define YF_PEND_TC   0x01 /* 'rect' changed, 'verts.tc' not up to date */
#define YF_PEND_CLR  0x02 /* 'verts.clr' set but 'mesh' not up to date */
    unsigned pend_mask;
    /* TODO: Other quad properties. */
};

/* Initializes a quad's mesh rectangle. */
static int init_rect(yf_quad_t *quad)
{
    assert(quad != NULL);

#ifdef YF_FLIP_TEX
# define YF_TEX_T 0.0f
#else
# define YF_TEX_T 1.0f
#endif

    static const float pos[] = {
        -1.0f, -1.0f, 0.5f,
        -1.0f, 1.0f, 0.5f,
        1.0f, 1.0f, 0.5f,
        1.0f, -1.0f, 0.5f
    };

    static const float tc[] = {
        0.0f, 1.0f - YF_TEX_T,
        0.0f, YF_TEX_T,
        1.0f, YF_TEX_T,
        1.0f, 1.0f - YF_TEX_T
    };

    static const float clr[] = {
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f
    };

    static const unsigned short indx[] = {
        0, 1, 2,
        0, 2, 3
    };

    static_assert(sizeof pos + sizeof tc + sizeof clr == sizeof quad->verts,
                  "!sizeof");

    memcpy(quad->verts, pos, sizeof pos);
    memcpy((char *)quad->verts + sizeof pos, tc, sizeof tc);
    memcpy((char *)quad->verts + sizeof pos + sizeof tc, clr, sizeof clr);

    void *dt = malloc(sizeof quad->verts + sizeof indx);
    if (dt == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    memcpy(dt, quad->verts, sizeof quad->verts);
    memcpy((char *)dt + sizeof quad->verts, indx, sizeof indx);

    const yf_meshdt_t data = {
        .prims = &(yf_primdt_t){
            .topology = YF_TOPOLOGY_TRIANGLE,
            .vert_n = 4,
            .indx_n = 6,
            .data_off = 0,
            .vsemt_mask = YF_VSEMT_POS | YF_VSEMT_TC | YF_VSEMT_CLR,
            .attrs =  (yf_attrdt_t[]){
                [0] = {YF_VSEMT_POS, YF_VFMT_FLOAT3, 0},
                [1] = {YF_VSEMT_TC, YF_VFMT_FLOAT2, sizeof pos},
                [2] = {YF_VSEMT_CLR, YF_VFMT_FLOAT4, sizeof pos + sizeof tc}
            },
            .attr_n = 3,
            .itype = YF_ITYPE_USHORT,
            .indx_data_off = sizeof quad->verts,
            .matl = NULL
        },
        .prim_n = 1,
        .data = dt,
        .data_sz = sizeof quad->verts + sizeof indx
    };

    quad->mesh = yf_mesh_init(&data);
    free(dt);
    return quad->mesh == NULL ? -1 : 0;
}

/* Updates a quad's mesh rectangle. */
static void update_rect(yf_quad_t *quad)
{
    assert(quad != NULL);
    assert(quad->pend_mask != YF_PEND_NONE);

    if (quad->pend_mask & YF_PEND_TC) {
        float s0, t0, s1, t1;

        if (quad->rect.size.width == 0) {
            s0 = t0 = 0.0f;
            s1 = t1 = 1.0f;
        } else {
            /* XXX: This assumes that the rect values are valid. */
            assert(quad->tex != NULL);

            const yf_dim2_t dim = yf_texture_getdim(quad->tex);
            const float wdt = dim.width;
            const float hgt = dim.height;
            s0 = quad->rect.origin.x / wdt;
            t0 = quad->rect.origin.y / hgt;
            s1 = quad->rect.size.width / wdt + s0;
            t1 = quad->rect.size.height / hgt + t0;
        }

#ifdef YF_FLIP_TEX
        const float tmp = t0;
        t0 = t1;
        t1 = tmp;
#endif

        float *tc = quad->verts + YF_VQUAD_POSN;

        tc[0] = s0;
        tc[1] = t0;

        tc[2] = s0;
        tc[3] = t1;

        tc[4] = s1;
        tc[5] = t1;

        tc[6] = s1;
        tc[7] = t0;
    }

    size_t off = YF_VQUAD_POSN * sizeof(float);
    float *beg = quad->verts + YF_VQUAD_POSN;
    size_t sz;

    if (quad->pend_mask & YF_PEND_TC) {
        sz = sizeof(float) * (quad->pend_mask & YF_PEND_CLR ?
                              YF_VQUAD_TCN + YF_VQUAD_CLRN :
                              YF_VQUAD_TCN);
    } else {
        off += YF_VQUAD_TCN * sizeof(float);
        beg += YF_VQUAD_TCN;
        sz = YF_VQUAD_CLRN * sizeof(float);
    }

#ifdef YF_DEVEL
    if (yf_mesh_setdata(quad->mesh, off, beg, sz) != 0)
        assert(0);
#else
    yf_mesh_setdata(quad->mesh, off, beg, sz);
#endif
}

/* Quad deinitialization callback. */
static void deinit_quad(void *quad)
{
    yf_mesh_deinit(((yf_quad_t *)quad)->mesh);
    free(quad);
}

yf_quad_t *yf_quad_init(void)
{
    yf_quad_t *quad = calloc(1, sizeof(yf_quad_t));
    if (quad == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    if ((quad->node = yf_node_init()) == NULL) {
        free(quad);
        return NULL;
    }
    yf_node_setobj(quad->node, YF_NODEOBJ_QUAD, quad, deinit_quad);

    if (init_rect(quad) != 0) {
        yf_quad_deinit(quad);
        return NULL;
    }
    return quad;
}

yf_node_t *yf_quad_getnode(yf_quad_t *quad)
{
    assert(quad != NULL);
    return quad->node;
}

yf_mesh_t *yf_quad_getmesh(yf_quad_t *quad)
{
    assert(quad != NULL);

    if (quad->pend_mask != YF_PEND_NONE) {
        update_rect(quad);
        quad->pend_mask = YF_PEND_NONE;
    }
    return quad->mesh;
}

yf_texture_t *yf_quad_gettex(yf_quad_t *quad)
{
    assert(quad != NULL);
    return quad->tex;
}

void yf_quad_settex(yf_quad_t *quad, yf_texture_t *tex)
{
    assert(quad != NULL);

    quad->tex = tex;
    quad->rect.origin = (yf_off2_t){0};
    quad->rect.size = tex != NULL ? yf_texture_getdim(tex) : (yf_dim2_t){0};
    quad->pend_mask |= YF_PEND_TC;
}

const yf_rect_t *yf_quad_getrect(yf_quad_t *quad)
{
    assert(quad != NULL);
    return &quad->rect;
}

void yf_quad_setrect(yf_quad_t *quad, const yf_rect_t *rect)
{
    assert(quad != NULL);
    assert(rect != NULL);

    memcpy(&quad->rect, rect, sizeof *rect);
    quad->pend_mask |= YF_PEND_TC;
}

yf_color_t yf_quad_getcolor(yf_quad_t *quad, int corner)
{
    assert(quad != NULL);

    float *clr = quad->verts + YF_VQUAD_POSN + YF_VQUAD_TCN;
    switch (corner) {
    case YF_CORNER_TOPL:
    case YF_CORNER_TOP:
    case YF_CORNER_LEFT:
    case YF_CORNER_ALL:
        break;
    case YF_CORNER_TOPR:
    case YF_CORNER_RIGHT:
        clr += 3 << 2;
        break;
    case YF_CORNER_BOTTOML:
    case YF_CORNER_BOTTOM:
        clr += 1 << 2;
        break;
    case YF_CORNER_BOTTOMR:
        clr += 2 << 2;
        break;
    }

    return (yf_color_t){clr[0], clr[1], clr[2], clr[3]};
}

void yf_quad_setcolor(yf_quad_t *quad, unsigned corner_mask, yf_color_t color)
{
    assert(quad != NULL);

    if (corner_mask & YF_CORNER_TOPL) {
        float *clr = quad->verts + YF_VQUAD_POSN + YF_VQUAD_TCN;
        clr[0] = color.r;
        clr[1] = color.g;
        clr[2] = color.b;
        clr[3] = color.a;
    }
    if (corner_mask & YF_CORNER_TOPR) {
        float *clr = quad->verts + YF_VQUAD_POSN + YF_VQUAD_TCN + (3 << 2);
        clr[0] = color.r;
        clr[1] = color.g;
        clr[2] = color.b;
        clr[3] = color.a;
    }
    if (corner_mask & YF_CORNER_BOTTOML) {
        float *clr = quad->verts + YF_VQUAD_POSN + YF_VQUAD_TCN + (1 << 2);
        clr[0] = color.r;
        clr[1] = color.g;
        clr[2] = color.b;
        clr[3] = color.a;
    }
    if (corner_mask & YF_CORNER_BOTTOMR) {
        float *clr = quad->verts + YF_VQUAD_POSN + YF_VQUAD_TCN + (2 << 2);
        clr[0] = color.r;
        clr[1] = color.g;
        clr[2] = color.b;
        clr[3] = color.a;
    }

    quad->pend_mask |= YF_PEND_CLR;
}

void yf_quad_deinit(yf_quad_t *quad)
{
    if (quad != NULL)
        yf_node_deinit(quad->node);
}
