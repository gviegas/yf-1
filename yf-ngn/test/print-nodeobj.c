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

void yf_print_nodeobj(yf_node_t *node)
{
    printf("\n[YF] OUTPUT (%s):\n", __func__);

    void *obj;
    switch (yf_node_getobj(node, &obj)) {
    case YF_NODEOBJ_NONE:
        printf(" (no object for this node)\n");
        break;

    case YF_NODEOBJ_MODEL: {
        yf_skeleton_t *skel = NULL;
        yf_skin_t *skin = yf_model_getskin(obj, &skel);
        printf(" NODEOBJ_MODEL <%p>:\n"
               "  mesh: <%p>\n"
               "  skin: <%p>\n"
               "  skel: <%p>\n",
               obj, (void *)yf_model_getmesh(obj), (void *)skin, (void *)skel);
    } break;

    case YF_NODEOBJ_TERRAIN: {
        printf(" NODEOBJ_TERRAIN <%p>:\n"
               "  mesh: <%p>\n"
               "  tex: <%p>\n"
               "  hmap: <%p>\n",
               obj, (void *)yf_terrain_getmesh(obj),
               (void *)yf_terrain_gettex(obj), (void *)yf_terrain_gethmap(obj));
    } break;

    case YF_NODEOBJ_PARTICLE: {
        yf_psys_t *sys = yf_particle_getsys(obj);
        printf(" NODEOBJ_PARTICLE <%p>:\n"
               "  mesh: <%p>\n"
               "  tex: <%p>\n",
               obj, (void *)yf_particle_getmesh(obj),
               (void *)yf_particle_gettex(obj));
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
        printf(" NODEOBJ_QUAD <%p>:\n"
               "  mesh: <%p>\n"
               "  tex: <%p>\n",
               obj, (void *)yf_quad_getmesh(obj), (void *)yf_quad_gettex(obj));
    } break;

    case YF_NODEOBJ_LABEL: {
        size_t n;
        yf_label_getstr(obj, NULL, &n);
        wchar_t str[n];
        yf_label_getstr(obj, str, &n);
        yf_dim2_t dim = yf_label_getdim(obj);
        printf(" NODEOBJ_LABEL <%p>:\n"
               "  mesh: <%p>\n"
               "  tex: <%p>\n"
               "  font: <%p>\n"
               "  pt: %hu\n"
               "  str: '%ls' (%zu)\n"
               "  dim: %ux%u\n",
               obj, (void *)yf_label_getmesh(obj),
               (void *)yf_label_gettex(obj), (void *)yf_label_getfont(obj),
               yf_label_getpt(obj), str, n, dim.width, dim.height);
    } break;

    case YF_NODEOBJ_LIGHT: {
        int lightt;
        yf_vec3_t color;
        float intensity, range, inner_angle, outer_angle;
        yf_light_getval(obj, &lightt, color, &intensity, &range,
                        &inner_angle, &outer_angle);
        printf(" NODEOBJ_LIGHT <%p>:\n"
               "  light type: LIGHTT_%s\n"
               "  color: [%.4f, %.4f, %.4f]\n"
               "  intensity: %.4f\n"
               "  range: %.4f\n"
               "  inner_angle: %.4f\n"
               "  outer_angle: %.4f\n",
               obj, (lightt == YF_LIGHTT_POINT ? "POINT" :
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
