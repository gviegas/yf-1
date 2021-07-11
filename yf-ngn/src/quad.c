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

struct YF_quad_o {
    YF_node node;
    YF_vquad verts[4];
    YF_mesh mesh;
    YF_texture tex;
    YF_rect rect;
#define YF_PEND_NONE 0
#define YF_PEND_TC   0x01 /* 'rect' changed, 'verts[].tc' not up to date */
#define YF_PEND_CLR  0x02 /* 'verts[].clr' set but 'mesh' not up to date */
    unsigned pend_mask;
    /* TODO: Other quad properties. */
};

/* Initializes a quad's mesh rectangle. */
static int init_rect(YF_quad quad)
{
    assert(quad != NULL);

#ifdef YF_FLIP_TEX
# define YF_TEX_T 0.0
#else
# define YF_TEX_T 1.0
#endif

    static const YF_vquad verts[4] = {
        {
            .pos = {-1.0, -1.0, 0.5},
            .tc = {0.0, 1.0-YF_TEX_T},
            .clr = {1.0, 1.0, 1.0, 1.0}
        },
        {
            .pos = {-1.0, 1.0, 0.5},
            .tc = {0.0, YF_TEX_T},
            .clr = {1.0, 1.0, 1.0, 1.0}
        },
        {
            .pos = {1.0, 1.0, 0.5},
            .tc = {1.0, YF_TEX_T},
            .clr = {1.0, 1.0, 1.0, 1.0}
        },
        {
            .pos = {1.0, -1.0, 0.5},
            .tc = {1.0, 1.0-YF_TEX_T},
            .clr = {1.0, 1.0, 1.0, 1.0}
        }
    };
    static const unsigned short inds[6] = {0, 1, 2, 0, 2, 3};

    const YF_meshdt data = {
        .v = {YF_VTYPE_QUAD, (void *)verts, 4},
        .i = {(void *)inds, sizeof(inds[0]), 6}
    };

    quad->mesh = yf_mesh_initdt(&data);
    memcpy(quad->verts, verts, sizeof verts);
    return quad->mesh == NULL ? -1 : 0;
}

/* Updates a quad's mesh rectangle. */
static void update_rect(YF_quad quad)
{
    assert(quad != NULL);
    assert(quad->pend_mask != YF_PEND_NONE);

    if (quad->pend_mask & YF_PEND_TC) {
        YF_float s0, t0, s1, t1;

        if (quad->rect.size.width == 0) {
            s0 = t0 = 0.0;
            s1 = t1 = 1.0;
        } else {
            /* XXX: This assumes that the rect values are valid. */
            assert(quad->tex != NULL);

            const YF_dim2 dim = yf_texture_getdim(quad->tex);
            const YF_float wdt = dim.width;
            const YF_float hgt = dim.height;
            s0 = quad->rect.origin.x / wdt;
            t0 = quad->rect.origin.y / hgt;
            s1 = quad->rect.size.width / wdt + s0;
            t1 = quad->rect.size.height / hgt + t0;
        }

#ifdef YF_FLIP_TEX
        const YF_float tmp = t0;
        t0 = t1;
        t1 = tmp;
#endif

        quad->verts[0].tc[0] = s0;
        quad->verts[0].tc[1] = t0;

        quad->verts[1].tc[0] = s0;
        quad->verts[1].tc[1] = t1;

        quad->verts[2].tc[0] = s1;
        quad->verts[2].tc[1] = t1;

        quad->verts[3].tc[0] = s1;
        quad->verts[3].tc[1] = t0;
    }

    const YF_slice range = {0, 4};
#ifdef YF_DEVEL
    if (yf_mesh_setvtx(quad->mesh, range, quad->verts) != 0) assert(0);
#else
    yf_mesh_setvtx(quad->mesh, range, quad->verts);
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

    unsigned i = 0;
    switch (corner) {
    case YF_CORNER_TOPL:
    case YF_CORNER_TOP:
    case YF_CORNER_LEFT:
    case YF_CORNER_ALL:
        i = 0;
        break;
    case YF_CORNER_TOPR:
    case YF_CORNER_RIGHT:
        i = 3;
        break;
    case YF_CORNER_BOTTOML:
    case YF_CORNER_BOTTOM:
        i = 1;
        break;
    case YF_CORNER_BOTTOMR:
        i = 2;
        break;
    }

    const YF_vquad *v = quad->verts+i;
    return (YF_color){v->clr[0], v->clr[1], v->clr[2], v->clr[3]};
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
