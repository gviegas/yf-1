/*
 * YF
 * light.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf-light.h"

struct YF_light_o {
    YF_node node;
    int lightt;
    YF_vec3 color;
    float intensity;
    float range;
    float inner_angle;
    float outer_angle;
};

YF_light yf_light_init(int lightt, const YF_vec3 color, float intensity,
                       float range, float inner_angle, float outer_angle)
{
    /* TODO */
    return NULL;
}

YF_node yf_light_getnode(YF_light light)
{
    /* TODO */
    return NULL;
}

void yf_light_deinit(YF_light light)
{
    /* TODO */
}
