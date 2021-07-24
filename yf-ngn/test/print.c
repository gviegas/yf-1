/*
 * YF
 * print.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "print.h"
#include "yf-model.h"
#include "yf-terrain.h"
#include "yf-particle.h"
#include "yf-quad.h"
#include "yf-label.h"
#include "mesh.h"

#undef YF_PTITLE
#define YF_PTITLE printf("\n[YF] OUTPUT (%s):", __func__);

void yf_print_nodeobj(YF_node node)
{
    YF_PTITLE;

    void *obj;
    switch (yf_node_getobj(node, &obj)) {
    case YF_NODEOBJ_NONE:
        printf("\nno object for this node");
        break;
    case YF_NODEOBJ_MODEL:
        printf("\nnodeobj is a model (%p)", obj);
        printf("\n mesh: %p", (void *)yf_model_getmesh((YF_model)obj));
        printf("\n matl:  %p", (void *)yf_model_getmatl((YF_model)obj));
        break;
    case YF_NODEOBJ_TERRAIN:
        printf("\nnodeobj is a terrain (%p)", obj);
        printf("\n mesh: %p", (void *)yf_terrain_getmesh((YF_terrain)obj));
        printf("\n tex:  %p", (void *)yf_terrain_gettex((YF_terrain)obj));
        printf("\n hmap: %p", (void *)yf_terrain_gethmap((YF_terrain)obj));
        break;
    case YF_NODEOBJ_PARTICLE:
        printf("\nnodeobj is a particle (%p)", obj);
        printf("\n mesh: %p", (void *)yf_particle_getmesh((YF_particle)obj));
        printf("\n tex:  %p", (void *)yf_particle_gettex((YF_particle)obj));
        /* TODO: Print the 'psys' parameters. */
        break;
    case YF_NODEOBJ_QUAD:
        printf("\nnodeobj is a quad (%p)", obj);
        printf("\n mesh: %p", (void *)yf_quad_getmesh((YF_quad)obj));
        printf("\n tex:  %p", (void *)yf_quad_gettex((YF_quad)obj));
        break;
    case YF_NODEOBJ_LABEL:
        printf("\nnodeobj is a label (%p)", obj);
        printf("\n mesh: %p", (void *)yf_label_getmesh((YF_label)obj));
        printf("\n tex:  %p", (void *)yf_label_gettex((YF_label)obj));
        printf("\n font: %p", (void *)yf_label_getfont((YF_label)obj));
        printf("\n pt:   %hu", yf_label_getpt((YF_label)obj));
        {
            size_t n;
            yf_label_getstr((YF_label)obj, NULL, &n);
            wchar_t str[n];
            yf_label_getstr((YF_label)obj, str, &n);
            printf("\n str:\n  '%ls' (%zu)", str, n);
            YF_dim2 dim = yf_label_getdim((YF_label)obj);
            printf("\n dim:  %ux%u", dim.width, dim.height);
        }
        break;
    case YF_NODEOBJ_LIGHT:
        /* TODO */
        assert(0);
        break;
    case YF_NODEOBJ_EFFECT:
        /* TODO */
        assert(0);
        break;
    default:
        assert(0);
    }

    printf("\n\n");
}

void yf_print_meshdt(const YF_meshdt *data)
{
#define YF_VMDL_PRINT(vtx) do { \
    printf("\n pos:  [%.3f %.3f %.3f]", \
           (vtx).pos[0], (vtx).pos[1], (vtx).pos[2]); \
    printf("\n tc:   [%.3f %.3f]", (vtx).tc[0], (vtx).tc[1]); \
    printf("\n norm: [%.3f %.3f %.3f]", \
           (vtx).norm[0], (vtx).norm[1], (vtx).norm[2]); \
    printf("\n"); } while (0)

#define YF_VTERR_PRINT(vtx) do { \
    printf("\n pos:  [%.3f %.3f %.3f]", \
           (vtx).pos[0], (vtx).pos[1], (vtx).pos[2]); \
    printf("\n tc:   [%.3f %.3f]", (vtx).tc[0], (vtx).tc[1]); \
    printf("\n norm: [%.3f %.3f %.3f]", \
           (vtx).norm[0], (vtx).norm[1], (vtx).norm[2]); \
    printf("\n"); } while (0)

#define YF_VPART_PRINT(vtx) do { \
    printf("\n pos:  [%.3f %.3f %.3f]", \
           (vtx).pos[0], (vtx).pos[1], (vtx).pos[2]); \
    printf("\n clr: [%.3f %.3f %.3f %.3f]", \
           (vtx).clr[0], (vtx).clr[1], (vtx).clr[2], (vtx).clr[3]); \
    printf("\n"); } while (0)

#define YF_VQUAD_PRINT(vtx) do { \
    printf("\n pos: [%.3f %.3f %.3f]", \
           (vtx).pos[0], (vtx).pos[1], (vtx).pos[2]); \
    printf("\n tc: [%.3f %.3f]", (vtx).tc[0], (vtx).tc[1]); \
    printf("\n clr: [%.3f %.3f %.3f %.3f]", \
           (vtx).clr[0], (vtx).clr[1], (vtx).clr[2], (vtx).clr[3]); \
    printf("\n"); } while (0)

#define YF_VLABL_PRINT(vtx) do { \
    printf("\n pos: [%.3f %.3f %.3f]", \
           (vtx).pos[0], (vtx).pos[1], (vtx).pos[2]); \
    printf("\n tc: [%.3f %.3f]", (vtx).tc[0], (vtx).tc[1]); \
    printf("\n clr: [%.3f %.3f %.3f %.3f]", \
           (vtx).clr[0], (vtx).clr[1], (vtx).clr[2], (vtx).clr[3]); \
    printf("\n"); } while (0)

    YF_PTITLE;

    printf("\nv.n: %zu", data->v.n);
    switch (data->v.vtype) {
    case YF_VTYPE_MDL:
        for (size_t i = 0; i < data->v.n; i++)
            YF_VMDL_PRINT(((YF_vmdl *)data->v.data)[i]);
        break;
    case YF_VTYPE_TERR:
        for (size_t i = 0; i < data->v.n; i++)
            YF_VTERR_PRINT(((YF_vterr *)data->v.data)[i]);
        break;
    case YF_VTYPE_PART:
        for (size_t i = 0; i < data->v.n; i++)
            YF_VPART_PRINT(((YF_vpart *)data->v.data)[i]);
        break;
    case YF_VTYPE_QUAD:
        for (size_t i = 0; i < data->v.n; i++)
            YF_VQUAD_PRINT(((YF_vquad *)data->v.data)[i]);
        break;
    case YF_VTYPE_LABL:
        for (size_t i = 0; i < data->v.n; i++)
            YF_VLABL_PRINT(((YF_vlabl *)data->v.data)[i]);
        break;
    default:
        assert(0);
    }

    const char *itype;
    if (data->i.itype == YF_ITYPE_USHORT)
        itype = "USHORT";
    else if (data->i.itype == YF_ITYPE_UINT)
        itype = "UINT";
    else
        itype = "(invalid ITYPE)";
    printf("\ni.itype: %s", itype);
    printf("\ni.n: %zu", data->i.n);
    for (size_t i = 0; i < data->i.n; i++) {
        if (i % 3 == 0)
            printf("\n");
        if (data->i.itype == YF_ITYPE_USHORT)
            printf(" %d", ((unsigned short *)data->i.data)[i]);
        else if (data->i.itype == YF_ITYPE_UINT)
            printf(" %u", ((unsigned *)data->i.data)[i]);
        else
            break;
    }

    printf("\n\n");
}


void yf_print_texdt(const YF_texdt *data)
{
    YF_PTITLE;

    printf("\ndim: %u, %u", data->dim.width, data->dim.height);

    switch (data->pixfmt) {
    case YF_PIXFMT_RGB8SRGB:
        printf("\npixfmt: %d (rgb8 sRGB)", data->pixfmt);
        printf("\ndata:");
        for (unsigned i = 0; i < data->dim.height; i++) {
            for (unsigned j = 0; j < data->dim.width; j++)
                printf("[%02x %02x %02x] ",
                       ((unsigned char *)data->data)[data->dim.width*3*i+3*j],
                       ((unsigned char *)data->data)[data->dim.width*3*i+3*j+1],
                       ((unsigned char *)data->data)[data->dim.width*3*i+3*j+2]
                       );
        }
        break;
    case YF_PIXFMT_RGBA8SRGB:
        printf("\npixfmt: %d (rgba8 sRGB)", data->pixfmt);
        printf("\ndata:");
        for (unsigned i = 0; i < data->dim.height; i++) {
            for (unsigned j = 0; j < data->dim.width; j++)
                printf("[%02x %02x %02x %02x] ",
                       ((unsigned char *)data->data)[data->dim.width*4*i+4*j],
                       ((unsigned char *)data->data)[data->dim.width*4*i+4*j+1],
                       ((unsigned char *)data->data)[data->dim.width*4*i+4*j+2],
                       ((unsigned char *)data->data)[data->dim.width*4*i+4*j+3]
                       );
        }
        break;
    default:
        printf("\npixfmt: %d", data->pixfmt);
        printf("\n*output unavailable*");
    }

    printf("\n\n");
}
