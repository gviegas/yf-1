/*
 * YF
 * print-nodeobj.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "yf-model.h"
#include "yf-terrain.h"
#include "yf-particle.h"
#include "yf-quad.h"
#include "yf-label.h"
#include "yf-light.h"

void yf_print_nodeobj(YF_node node)
{
    printf("\n[YF] OUTPUT (%s):\n", __func__);

    void *obj;
    switch (yf_node_getobj(node, &obj)) {
    case YF_NODEOBJ_NONE:
        printf(" (no object for this node)\n");
        break;

    case YF_NODEOBJ_MODEL: {
        YF_model mdl = obj;
        YF_skeleton skel = NULL;
        YF_skin skin = yf_model_getskin(mdl, &skel);
        printf(" NODEOBJ_MODEL <%p>:\n"
               "  mesh: %p\n"
               "  skin: %p\n"
               "  skel: %p\n"
               "  matl: %p\n",
               (void *)mdl, (void *)yf_model_getmesh(mdl), (void *)skin,
               (void *)skel, (void *)yf_model_getmatl(mdl));
    } break;

    case YF_NODEOBJ_TERRAIN: {
        YF_terrain terr = obj;
        printf(" NODEOBJ_TERRAIN <%p>:\n"
               "  mesh: %p\n"
               "  tex:  %p\n"
               "  hmap: %p\n",
               (void *)terr, (void *)yf_terrain_getmesh(terr),
               (void *)yf_terrain_gettex(terr),
               (void *)yf_terrain_gethmap(terr));
    } break;

    case YF_NODEOBJ_PARTICLE: {
        YF_particle part = obj;
        printf(" NODEOBJ_PARTICLE <%p>:\n"
               "  mesh: %p\n"
               "  tex:  %p\n",
               (void *)part, (void *)yf_particle_getmesh(part),
               (void *)yf_particle_gettex(part));
    } break;

    case YF_NODEOBJ_QUAD: {
        YF_quad quad = obj;
        printf(" NODEOBJ_QUAD <%p>:\n"
               "  mesh: %p\n"
               "  tex:  %p\n",
               (void *)quad, (void *)yf_quad_getmesh(quad),
               (void *)yf_quad_gettex(quad));
    } break;

    case YF_NODEOBJ_LABEL: {
        YF_label labl = obj;
        size_t n;
        yf_label_getstr(labl, NULL, &n);
        wchar_t str[n];
        yf_label_getstr(labl, str, &n);
        YF_dim2 dim = yf_label_getdim(labl);
        printf(" NODEOBJ_LABEL <%p>:\n"
               "  mesh: %p\n"
               "  tex:  %p\n"
               "  font: %p\n"
               "  pt:   %hu\n"
               "  str:  '%ls' (%zu)\n"
               "  dim:  %ux%u\n",
               (void *)labl, (void *)yf_label_getmesh(labl),
               (void *)yf_label_gettex(labl), (void *)yf_label_getfont(labl),
               yf_label_getpt(labl), str, n, dim.width, dim.height);
    } break;

    case YF_NODEOBJ_LIGHT: {
        YF_light light = obj;
        int lightt;
        YF_vec3 color;
        float intensity, range, inner_angle, outer_angle;
        yf_light_getval(light, &lightt, color, &intensity, &range,
                        &inner_angle, &outer_angle);
        printf(" NODEOBJ_LIGHT <%p>:\n"
               "  light type:  LIGHTT_%s\n"
               "  color:       [%.4f, %.4f, %.4f]\n"
               "  intensity:   %.4f\n"
               "  range:       %.4f\n"
               "  inner_angle: %.4f\n"
               "  outer_angle: %.4f\n",
               (void *)light, (lightt == YF_LIGHTT_POINT ? "POINT" :
                               (lightt == YF_LIGHTT_SPOT ? "SPOT" : "DIRECT")),
               color[0], color[1], color[2], intensity, range, inner_angle,
               outer_angle);
    } break;

    case YF_NODEOBJ_EFFECT:
        /* TODO */
        assert(0);
        break;

    default:
        assert(0);
    }

    puts("");
}
