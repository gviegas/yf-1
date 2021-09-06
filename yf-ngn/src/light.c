/*
 * YF
 * light.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "yf-light.h"
#include "node.h"

struct YF_light_o {
    YF_node node;
    int lightt;
    YF_vec3 color;
    float intensity;
    float range;
    float inner_angle;
    float outer_angle;
};

/* Light deinitialization callback. */
static void deinit_light(void *light)
{
    free(light);
}

YF_light yf_light_init(int lightt, const YF_vec3 color, float intensity,
                       float range, float inner_angle, float outer_angle)
{
    YF_light light = calloc(1, sizeof(struct YF_light_o));
    if (light == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    light->node = yf_node_init();
    if (light->node == NULL) {
        free(light);
        return NULL;
    }
    yf_node_setobj(light->node, YF_NODEOBJ_LIGHT, light, deinit_light);

    light->lightt = lightt;
    yf_vec3_copy(light->color, color);
    light->intensity = intensity;
    light->range = range;
    light->inner_angle = inner_angle;
    light->outer_angle = outer_angle;

    return light;
}

YF_node yf_light_getnode(YF_light light)
{
    assert(light != NULL);
    return light->node;
}

void yf_light_deinit(YF_light light)
{
    if (light != NULL)
        yf_node_deinit(light->node);
}
