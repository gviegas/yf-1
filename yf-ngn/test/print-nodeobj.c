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
               "  skel: %p\n",
               (void *)mdl, (void *)yf_model_getmesh(mdl), (void *)skin,
               (void *)skel);
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
        YF_psys *sys = yf_particle_getsys(part);
        printf(" NODEOBJ_PARTICLE <%p>:\n"
               "  mesh: %p\n"
               "  tex:  %p\n",
               (void *)part, (void *)yf_particle_getmesh(part),
               (void *)yf_particle_gettex(part));
        printf("  sys:\n"
               "   emitter:\n"
               "    norm: [%.4f, %.4f, %.4f]\n"
               "    size: %.4f\n"
               "   lifetime:\n"
               "    duration min: %.4f\n"
               "    duration max: %.4f\n"
               "    spawn min: %.4f\n"
               "    spawn max: %.4f\n"
               "    death min: %.4f\n"
               "    death max: %.4f\n"
               "    once: %s\n"
               "   color:\n"
               "    min: [%.4f, %.4f, %.4f]\n"
               "    max: [%.4f, %.4f, %.4f]\n"
               "   velocity:\n"
               "    min: [%.4f, %.4f, %.4f]\n"
               "    max: [%.4f, %.4f, %.4f]\n",
               sys->emitter.norm[0], sys->emitter.norm[1], sys->emitter.norm[2],
               sys->emitter.size, sys->lifetime.duration_min,
               sys->lifetime.duration_max, sys->lifetime.spawn_min,
               sys->lifetime.spawn_max, sys->lifetime.death_min,
               sys->lifetime.death_max, (sys->lifetime.once ? "yes" : "no"),
               sys->color.min[0], sys->color.min[1], sys->color.min[2],
               sys->color.max[0], sys->color.max[1], sys->color.max[2],
               sys->velocity.min[0], sys->velocity.min[1],
               sys->velocity.min[2], sys->velocity.max[0],
               sys->velocity.max[1], sys->velocity.max[2]);
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
