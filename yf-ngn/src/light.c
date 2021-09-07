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
    switch (lightt) {
    case YF_LIGHTT_POINT:
    case YF_LIGHTT_DIRECT:
        if (intensity < 0.0f) {
            yf_seterr(YF_ERR_INVARG, __func__);
            return NULL;
        }
        break;
    case YF_LIGHTT_SPOT:
        if (intensity < 0.0f ||
            inner_angle < 0.0f || inner_angle > 1.5707963268f ||
            outer_angle < 0.0f || outer_angle > 1.5707963268f) {

            yf_seterr(YF_ERR_INVARG, __func__);
            return NULL;
        }
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    YF_light light = malloc(sizeof(struct YF_light_o));
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

void yf_light_getval(YF_light light, int *lightt, YF_vec3 color,
                     float *intensity, float *range, float *inner_angle,
                     float *outer_angle)
{
    assert(light != NULL);

    if (lightt != NULL)
        *lightt = light->lightt;
    if (color != NULL)
        yf_vec3_copy(color, light->color);
    if (intensity != NULL)
        *intensity = light->intensity;
    if (range != NULL)
        *range = light->range;
    if (inner_angle != NULL)
        *inner_angle = light->inner_angle;
    if (outer_angle != NULL)
        *outer_angle = light->outer_angle;
}

int yf_light_setpoint(YF_light light, const YF_vec3 color, float intensity,
                      float range)
{
    assert(light != NULL);

    if (intensity < 0.0f) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    light->lightt = YF_LIGHTT_POINT;
    yf_vec3_copy(light->color, color);
    light->intensity = intensity;
    light->range = range;

    return 0;
}

void yf_light_deinit(YF_light light)
{
    if (light != NULL)
        yf_node_deinit(light->node);
}
