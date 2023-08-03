/*
 * YF
 * material.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#define METHOD_PBRSG 0
#define METHOD_PBRMR 1
#define METHOD_UNLIT 2

#define BLEND_OPAQUE 0
#define BLEND_BLEND  1

#define TEX_CLR  0x01
#define TEX_PBR  0x02
#define TEX_NORM 0x04
#define TEX_OCC  0x08
#define TEX_EMIS 0x10

#define PI          3.14159265358979323846
#define ONE_OVER_PI 0.31830988618379067154

layout(std140, column_major) uniform;

/**
 * Material.
 */
/* TODO: Fix alignment. */
layout(set=1, binding=1) uniform umatl {
    int method;
    int blend;
    float norm_fac;
    float occ_fac;
    vec4 clr_fac;
    vec4 pbr_fac;
    vec3 emis_fac;
    uint tex_mask;
} matl_;

/**
 * Textures.
 */
layout(set=1, binding=2) uniform sampler2D clr_is_;
layout(set=1, binding=3) uniform sampler2D pbr_is_;
layout(set=1, binding=4) uniform sampler2D norm_is_;
layout(set=1, binding=5) uniform sampler2D occ_is_;
layout(set=1, binding=6) uniform sampler2D emis_is_;

/**
 * Fresnel term (F).
 */
vec3 fresnel_f(vec3 f0, vec3 f90, float vdoth)
{
    return f0 + (f90 - f0) * pow(1.0 - abs(vdoth), 5.0);
}

/**
 * Visibility (V).
 */
float visibility_v(float ndotv, float ndotl, float arxar)
{
    float diffa = 1.0 - arxar;
    float ggx = sqrt(ndotv * ndotv * diffa + arxar) * ndotl +
                sqrt(ndotl * ndotl * diffa + arxar) * ndotv;

    return (ggx > 0.0) ? (0.5 / ggx) : (0.0);
}

/**
 * Microfacet (D).
 */
float microfacet_d(float ndoth, float arxar)
{
    float fac = (ndoth * ndoth) * (arxar - 1.0) + 1.0;
    return arxar / (PI * fac * fac);
}

/**
 * Diffuse BRDF.
 */
vec3 diffuse_brdf(vec3 color, vec3 fterm)
{
    return (1.0 - fterm) * color * ONE_OVER_PI;
}

/**
 * Specular BRDF.
 */
vec3 specular_brdf(vec3 fterm, float ndotv, float ndotl, float ndoth,
                   float arxar)
{
    return fterm * visibility_v(ndotv, ndotl, arxar) * microfacet_d(ndoth,
                                                                    arxar);
}
