/*
 * YF
 * label.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"

#include "yf-label.h"
#include "node.h"
#include "mesh.h"
#include "texture.h"
#include "font.h"

/* TODO: Consider taking this values from the font instead. */
#define YF_FONTSZ_MIN 9
#define YF_FONTSZ_MAX 144
#ifndef YF_DPI
# define YF_DPI 72
#endif

#define YF_VLABL_POSN (3 << 2)
#define YF_VLABL_TCN  (2 << 2)
#define YF_VLABL_CLRN (4 << 2)
#define YF_VLABL_N    (YF_VLABL_POSN + YF_VLABL_TCN + YF_VLABL_CLRN)

struct yf_label {
    yf_node_t *node;
    float verts[YF_VLABL_N];
    yf_mesh_t *mesh;
    yf_fontrz_t rz;
    yf_font_t *font;
    wchar_t *str;
    unsigned short pt;
#define YF_PEND_NONE 0
#define YF_PEND_TC   0x01 /* 'rz' changed, 'verts.tc' not up to date */
#define YF_PEND_CLR  0x02 /* 'verts.clr' set but 'mesh' not up to date */
#define YF_PEND_RZ   0x04 /* font prop. changed, 'rz' not up to date */
    unsigned pend_mask;
};

/* Initializes a label's mesh rectangle. */
static int init_rect(yf_label_t *labl)
{
    assert(labl != NULL);

    static const float pos[] = {
        -1.0f, -1.0f, 0.5f,
        -1.0f, 1.0f, 0.5f,
        1.0f, 1.0f, 0.5f,
        1.0f, -1.0f, 0.5f
    };

    static const float tc[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
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

    static_assert(sizeof pos + sizeof tc + sizeof clr == sizeof labl->verts,
                  "!sizeof");

    memcpy(labl->verts, pos, sizeof pos);
    memcpy((char *)labl->verts + sizeof pos, tc, sizeof tc);
    memcpy((char *)labl->verts + sizeof pos + sizeof tc, clr, sizeof clr);

    void *dt = malloc(sizeof labl->verts + sizeof indx);
    if (dt == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    memcpy(dt, labl->verts, sizeof labl->verts);
    memcpy((char *)dt + sizeof labl->verts, indx, sizeof indx);

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
            .indx_data_off = sizeof labl->verts,
            .matl = NULL
        },
        .prim_n = 1,
        .data = dt,
        .data_sz = sizeof labl->verts + sizeof indx
    };

    labl->mesh = yf_mesh_init(&data);
    free(dt);
    return labl->mesh == NULL ? -1 : 0;
}

/* Updates a label's mesh rectangle. */
static void update_rect(yf_label_t *labl)
{
    assert(labl != NULL);
    assert(labl->pend_mask != YF_PEND_NONE);

    if (labl->pend_mask & YF_PEND_TC) {
        const yf_fontrz_t *rz = &labl->rz;
        float s0, t0, s1, t1;

        if (rz->tex == NULL || rz->dim.width == 0 || rz->dim.height == 0) {
            s0 = t0 = 0.0;
            s1 = t1 = 1.0;
        } else {
            const yf_dim2_t dim = yf_texture_getdim(rz->tex);
            const float wdt = dim.width;
            const float hgt = dim.height;
            s0 = rz->off.x / wdt;
            t0 = rz->off.y / hgt;
            s1 = rz->dim.width / wdt + s0;
            t1 = rz->dim.height / hgt + t0;
        }

        float *tc = labl->verts + YF_VLABL_POSN;

        tc[0] = s0;
        tc[1] = t1;

        tc[2] = s0;
        tc[3] = t0;

        tc[4] = s1;
        tc[5] = t0;

        tc[6] = s1;
        tc[7] = t1;
    }

    size_t off = YF_VLABL_POSN * sizeof(float);
    float *beg = labl->verts + YF_VLABL_POSN;
    size_t sz;

    if (labl->pend_mask & YF_PEND_TC) {
        sz = sizeof(float) * (labl->pend_mask & YF_PEND_CLR ?
                              YF_VLABL_TCN + YF_VLABL_CLRN :
                              YF_VLABL_TCN);
    } else {
        off += YF_VLABL_TCN * sizeof(float);
        beg += YF_VLABL_TCN;
        sz = YF_VLABL_CLRN * sizeof(float);
    }

#ifdef YF_DEVEL
    if (yf_mesh_setdata(labl->mesh, off, beg, sz) != 0)
        assert(0);
#else
    yf_mesh_setdata(labl->mesh, off, beg, sz);
#endif
}

/* Copies label glyphs to texture. */
static int copy_glyphs(yf_label_t *labl)
{
    assert(labl != NULL);
    assert(labl->font != NULL);
    assert(labl->pend_mask & YF_PEND_RZ);

    const wchar_t *str;
    if (labl->str == NULL)
        str = L"(nil)";
    else if (wcslen(labl->str) == 0)
        str = L"(empty)";
    else
        str = labl->str;

    return yf_font_rasterize(labl->font, str, labl->pt, YF_DPI, &labl->rz);
}

/* Label deinitialization callback. */
static void deinit_labl(void *obj)
{
    yf_label_t *labl = obj;

    yf_mesh_deinit(labl->mesh);

    if (labl->font != NULL)
        yf_font_yieldrz(labl->font, &labl->rz);

    free(labl->str);
    free(labl);
}

yf_label_t *yf_label_init(void)
{
    yf_label_t *labl = calloc(1, sizeof(yf_label_t));
    if (labl == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    if ((labl->node = yf_node_init()) == NULL) {
        free(labl);
        return NULL;
    }
    yf_node_setobj(labl->node, YF_NODEOBJ_LABEL, labl, deinit_labl);
    labl->pt = 16;
    labl->pend_mask = YF_PEND_RZ;

    if (init_rect(labl) != 0) {
        yf_label_deinit(labl);
        return NULL;
    }
    return labl;
}

yf_node_t *yf_label_getnode(yf_label_t *labl)
{
    assert(labl != NULL);
    return labl->node;
}

yf_mesh_t *yf_label_getmesh(yf_label_t *labl)
{
    assert(labl != NULL);

    if (labl->pend_mask != YF_PEND_NONE) {
        if (labl->pend_mask & YF_PEND_RZ) {
            copy_glyphs(labl);
            labl->pend_mask &= ~YF_PEND_RZ;
            labl->pend_mask |= YF_PEND_TC;
        }
        update_rect(labl);
        labl->pend_mask = YF_PEND_NONE;
    }
    return labl->mesh;
}

yf_texture_t *yf_label_gettex(yf_label_t *labl)
{
    assert(labl != NULL);

    if (labl->pend_mask & YF_PEND_RZ) {
        copy_glyphs(labl);
        labl->pend_mask &= ~YF_PEND_RZ;
        labl->pend_mask |= YF_PEND_TC;
    }
    return labl->rz.tex;
}

yf_font_t *yf_label_getfont(yf_label_t *labl)
{
    assert(labl != NULL);
    return labl->font;
}

void yf_label_setfont(yf_label_t *labl, yf_font_t *font)
{
    assert(labl != NULL);

    if (font == labl->font)
        return;
    if (labl->font != NULL)
        yf_font_yieldrz(labl->font, &labl->rz);

    labl->font = font;
    labl->pend_mask |= YF_PEND_RZ;
}

wchar_t *yf_label_getstr(yf_label_t *labl, wchar_t *dst, size_t *n)
{
    assert(labl != NULL);
    assert(n != NULL);
    assert(dst == NULL || *n > 0);

    if (dst == NULL) {
        *n = labl->str == NULL ? 1 : wcslen(labl->str) + 1;
        return NULL;
    }

    if (labl->str == NULL) {
        dst[0] = L'\0';
        *n = 1;
        return dst;
    }

    const size_t str_n = wcslen(labl->str) + 1;
    if (str_n <= *n) {
        *n = str_n;
        return wcscpy(dst, labl->str);
    }
    *n = str_n;
    return NULL;
}

int yf_label_setstr(yf_label_t *labl, const wchar_t *str)
{
    assert(labl != NULL);

    if (str == NULL) {
        free(labl->str);
        labl->str = NULL;
        labl->pend_mask |= YF_PEND_RZ;
        return 0;
    }

    const size_t len = wcslen(str);
    if (labl->str == NULL) {
        labl->str = malloc(sizeof(wchar_t) * (1+len));
        if (labl->str == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }
    } else {
        const size_t cur_len = wcslen(labl->str);
        if (cur_len != len) {
            void *tmp = realloc(labl->str, sizeof(wchar_t) * (1+len));
            if (tmp != NULL) {
                labl->str = tmp;
            } else if (cur_len < len) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                return -1;
            }
        }
    }
    wcscpy(labl->str, str);
    labl->pend_mask |= YF_PEND_RZ;
    return 0;
}

unsigned short yf_label_getpt(yf_label_t *labl)
{
    assert(labl != NULL);
    return labl->pt;
}

int yf_label_setpt(yf_label_t *labl, unsigned short pt)
{
    assert(labl != NULL);

    if (pt == labl->pt)
        return 0;

    if (pt < YF_FONTSZ_MIN || pt > YF_FONTSZ_MAX) {
        yf_seterr(YF_ERR_LIMIT, __func__);
        return -1;
    }
    labl->pt = pt;
    labl->pend_mask |= YF_PEND_RZ;
    return 0;
}

yf_color_t yf_label_getcolor(yf_label_t *labl, int corner)
{
    assert(labl != NULL);

    float *clr = labl->verts + YF_VLABL_POSN + YF_VLABL_TCN;
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

void yf_label_setcolor(yf_label_t *labl, unsigned corner_mask,
                       yf_color_t color)
{
    assert(labl != NULL);

    if (corner_mask & YF_CORNER_TOPL) {
        float *clr = labl->verts + YF_VLABL_POSN + YF_VLABL_TCN;
        clr[0] = color.r;
        clr[1] = color.g;
        clr[2] = color.b;
        clr[3] = color.a;
    }
    if (corner_mask & YF_CORNER_TOPR) {
        float *clr = labl->verts + YF_VLABL_POSN + YF_VLABL_TCN + (3 << 2);
        clr[0] = color.r;
        clr[1] = color.g;
        clr[2] = color.b;
        clr[3] = color.a;
    }
    if (corner_mask & YF_CORNER_BOTTOML) {
        float *clr = labl->verts + YF_VLABL_POSN + YF_VLABL_TCN + (1 << 2);
        clr[0] = color.r;
        clr[1] = color.g;
        clr[2] = color.b;
        clr[3] = color.a;
    }
    if (corner_mask & YF_CORNER_BOTTOMR) {
        float *clr = labl->verts + YF_VLABL_POSN + YF_VLABL_TCN + (2 << 2);
        clr[0] = color.r;
        clr[1] = color.g;
        clr[2] = color.b;
        clr[3] = color.a;
    }

    labl->pend_mask |= YF_PEND_CLR;
}

yf_dim2_t yf_label_getdim(yf_label_t *labl)
{
    assert(labl != NULL);

    if (labl->pend_mask & YF_PEND_RZ) {
        copy_glyphs(labl);
        labl->pend_mask &= ~YF_PEND_RZ;
        labl->pend_mask |= YF_PEND_TC;
    }
    return labl->rz.dim;
}

void yf_label_deinit(yf_label_t *labl)
{
    if (labl != NULL)
        yf_node_deinit(labl->node);
}
