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
