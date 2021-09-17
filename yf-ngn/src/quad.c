/*
 * YF
 * quad.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "yf-quad.h"
#include "node.h"
#include "mesh.h"
#include "resmgr.h"

#define YF_VQUAD_POSN (3 << 2)
#define YF_VQUAD_TCN  (2 << 2)
#define YF_VQUAD_CLRN (4 << 2)
#define YF_VQUAD_N    (YF_VQUAD_POSN + YF_VQUAD_TCN + YF_VQUAD_CLRN)

struct YF_quad_o {
    YF_node node;
    float verts[YF_VQUAD_N];
    YF_mesh mesh;
    YF_texture tex;
    YF_rect rect;
#define YF_PEND_NONE 0
#define YF_PEND_TC   0x01 /* 'rect' changed, 'verts.tc' not up to date */
#define YF_PEND_CLR  0x02 /* 'verts.clr' set but 'mesh' not up to date */
    unsigned pend_mask;
    /* TODO: Other quad properties. */
};

/* Initializes a quad's mesh rectangle. */
static int init_rect(YF_quad quad)
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

    void *buf = malloc(sizeof quad->verts + sizeof indx);
    if (buf == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    memcpy(buf, quad->verts, sizeof quad->verts);
    memcpy((char *)buf + sizeof quad->verts, indx, sizeof indx);

    const YF_meshdt data = {
        .prims = &(YF_primdt){
            .primitive = YF_PRIMITIVE_TRIANGLE,
            .vert_n = 4,
            .indx_n = 6,
            .data_off = 0,
            .attrs =  (YF_attrdt[]){
                [0] = {YF_RESLOC_POS, YF_VFMT_FLOAT3, 0},
                [1] = {YF_RESLOC_TC, YF_VFMT_FLOAT2, sizeof pos},
                [2] = {YF_RESLOC_CLR, YF_VFMT_FLOAT4, sizeof pos + sizeof tc}
            },
            .attr_n = 3,
            .itype = YF_ITYPE_USHORT,
            .indx_data_off = sizeof pos + sizeof tc + sizeof clr
        },
        .prim_n = 1,
        .data = buf,
        .data_sz = sizeof quad->verts + sizeof indx
    };

    quad->mesh = yf_mesh_initdt(&data);
    free(buf);
    return quad->mesh == NULL ? -1 : 0;
}

/* Updates a quad's mesh rectangle. */
static void update_rect(YF_quad quad)
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

            const YF_dim2 dim = yf_texture_getdim(quad->tex);
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
    yf_mesh_deinit(((YF_quad)quad)->mesh);
    free(quad);
}

YF_quad yf_quad_init(void)
{
    YF_quad quad = calloc(1, sizeof(struct YF_quad_o));
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

YF_node yf_quad_getnode(YF_quad quad)
{
    assert(quad != NULL);
    return quad->node;
}

YF_mesh yf_quad_getmesh(YF_quad quad)
{
    assert(quad != NULL);

    if (quad->pend_mask != YF_PEND_NONE) {
        update_rect(quad);
        quad->pend_mask = YF_PEND_NONE;
    }
    return quad->mesh;
}

YF_texture yf_quad_gettex(YF_quad quad)
{
    assert(quad != NULL);
    return quad->tex;
}

void yf_quad_settex(YF_quad quad, YF_texture tex)
{
    assert(quad != NULL);

    quad->tex = tex;
    quad->rect.origin = (YF_off2){0};
    quad->rect.size = tex != NULL ? yf_texture_getdim(tex) : (YF_dim2){0};
    quad->pend_mask |= YF_PEND_TC;
}

const YF_rect *yf_quad_getrect(YF_quad quad)
{
    assert(quad != NULL);
    return &quad->rect;
}

void yf_quad_setrect(YF_quad quad, const YF_rect *rect)
{
    assert(quad != NULL);
    assert(rect != NULL);

    memcpy(&quad->rect, rect, sizeof *rect);
    quad->pend_mask |= YF_PEND_TC;
}

YF_color yf_quad_getcolor(YF_quad quad, int corner)
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

    return (YF_color){clr[0], clr[1], clr[2], clr[3]};
}

void yf_quad_setcolor(YF_quad quad, unsigned corner_mask, YF_color color)
{
    assert(quad != NULL);

    if (corner_mask & YF_CORNER_TOPL) {
        quad->verts[0].clr[0] = color.r;
        quad->verts[0].clr[1] = color.g;
        quad->verts[0].clr[2] = color.b;
        quad->verts[0].clr[3] = color.a;
    }
    if (corner_mask & YF_CORNER_TOPR) {
        quad->verts[3].clr[0] = color.r;
        quad->verts[3].clr[1] = color.g;
        quad->verts[3].clr[2] = color.b;
        quad->verts[3].clr[3] = color.a;
    }
    if (corner_mask & YF_CORNER_BOTTOML) {
        quad->verts[1].clr[0] = color.r;
        quad->verts[1].clr[1] = color.g;
        quad->verts[1].clr[2] = color.b;
        quad->verts[1].clr[3] = color.a;
    }
    if (corner_mask & YF_CORNER_BOTTOMR) {
        quad->verts[2].clr[0] = color.r;
        quad->verts[2].clr[1] = color.g;
        quad->verts[2].clr[2] = color.b;
        quad->verts[2].clr[3] = color.a;
    }

    quad->pend_mask |= YF_PEND_CLR;
}

void yf_quad_deinit(YF_quad quad)
{
    if (quad != NULL)
        yf_node_deinit(quad->node);
}
