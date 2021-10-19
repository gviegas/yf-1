/*
 * YF
 * light.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef LIGHT_N
# error "LIGHT_N not defined"
#endif

#define TYPE_POINT  0
#define TYPE_SPOT   1
#define TYPE_DIRECT 2

layout(std140, column_major) uniform;

/**
 * Light source type.
 */
struct T_light {
    int unused;
    int type;
    float inten;
    float range;
    vec3 clr;
    float ang_scale;
    vec3 pos;
    float ang_off;
    vec3 dir;
};

/**
 * Light.
 */
layout(set=0, binding=1) uniform U_light {
    T_light l[LIGHT_N];
} light_;

/**
 * Range attenuation.
 */
float attenuation(float dist, float range)
{
    if (range > 0.0)
        return clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0) / pow(dist, 2.0);

    return 1.0 / pow(dist, 2.0);
}

/**
 * Angular attenuation.
 */
float attenuation(vec3 dir, vec3 l, float ang_scale, float ang_off)
{
    float atn = clamp(dot(dir, -l) * ang_scale + ang_off, 0.0, 1.0);
    return atn * atn;
}
