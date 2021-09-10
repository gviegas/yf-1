/*
 * YF
 * model.frag.glsl
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#version 460 core

#ifndef LIGHT_N
# error "LIGHT_N not defined"
#endif

#define TYPE_POINT  0
#define TYPE_SPOT   1
#define TYPE_DIRECT 2

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
 * Type defining a light source.
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
 * Material.
 */
layout(set=1, binding=1) uniform U_matl {
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

layout(location=0) in IO_v {
    vec3 pos;
    vec2 tc;
    vec3 norm;
    vec4 clr;
    vec3 eye;
} v_;

layout(location=0) out vec4 clr_;

/**
 * Range attenuation.
 */
float attenuation(float dist, float range)
{
    return range > 0.0 ?
           clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0) / pow(dist, 2.0) :
           1.0 / pow(dist, 2.0);
}

/**
 * Angular attenuation.
 */
float attenuation(vec3 dir, vec3 l, float ang_scale, float ang_off)
{
    float atn = clamp(dot(dir, -l) * ang_scale + ang_off, 0.0, 1.0);
    return atn * atn;
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
 * Visibility (V).
 */
float visibility_v(float ndotv, float ndotl, float arxar)
{
    float diffa = 1.0 - arxar;
    float ggx = ndotl * sqrt(ndotv * ndotv * diffa + arxar) +
                ndotv * sqrt(ndotl * ndotl * diffa + arxar);
    return (ggx > 0.0) ? (0.5 / ggx) : (0.0);
}

/**
 * Fresnel term (F).
 */
vec3 fresnel_f(vec3 f0, vec3 f90, float vdoth)
{
    return f0 + (f90 - f0) * pow(1.0 - abs(vdoth), 5.0);
}

/**
 * Diffuse BRDF.
 */
vec3 diffuse_brdf(vec3 color, vec3 fterm)
{
    return (1.0 - fterm) * (color * ONE_OVER_PI);
}

/**
 * Specular BRDF.
 */
vec3 specular_brdf(vec3 fterm, float ndotv, float ndotl, float ndoth,
                   float arxar)
{
    return fterm *
           visibility_v(ndotv, ndotl, arxar) *
           microfacet_d(ndoth, arxar);
}

/**
 * Gets fragment color.
 */
vec4 getclr()
{
    /* TODO: Normal/occlusion/emissive maps. */

    vec4 clr = v_.clr;

    if ((matl_.tex_mask & TEX_CLR) == TEX_CLR)
        clr *= textureLod(clr_is_, v_.tc, 0.0);
    clr *= matl_.clr_fac;

    vec3 albedo, f0, f90;
    float ar;

    switch (matl_.method) {
    case METHOD_PBRSG:
        vec3 specular = matl_.pbr_fac.rgb;
        float glossiness = matl_.pbr_fac.a;
        if ((matl_.tex_mask & TEX_PBR) == TEX_PBR) {
            vec4 spec_gloss = textureLod(pbr_is_, v_.tc, 0.0);
            specular *= spec_gloss.rgb;
            glossiness *= spec_gloss.a;
        }
        albedo = clr.rgb * (1.0 - max(specular.r, max(specular.g, specular.b)));
        f0 = specular;
        f90 = vec3(1.0);
        ar = 1.0 - glossiness;
        ar *= ar;
        break;

    case METHOD_PBRMR:
        float metallic = matl_.pbr_fac[0];
        float roughness = matl_.pbr_fac[1];
        if ((matl_.tex_mask & TEX_PBR) == TEX_PBR) {
            vec4 metal_rough = textureLod(pbr_is_, v_.tc, 0.0);
            metallic *= metal_rough.b;
            roughness *= metal_rough.g;
        }
        vec3 ior = vec3(0.04);
        albedo = mix(clr.rgb * (vec3(1.0) - ior), vec3(0.0), metallic);
        f0 = mix(ior, clr.rgb, metallic);
        f90 = vec3(1.0);
        ar = roughness * roughness;
        break;

    case METHOD_UNLIT:
        if ((matl_.blend == BLEND_OPAQUE))
            clr.a = 1.0;
        return clr;
    }

    clr.rgb = vec3(0.0);

    vec3 v = normalize(v_.eye);
    vec3 n = normalize(v_.norm);
    float ndotv = max(dot(n, v), 0.0);

    for (uint i = 0; i < LIGHT_N; i++) {
        if (light_.l[i].unused != 0)
            break;

        vec3 l;
        float dist = 0.0001;
        float atn = 1.0;

        switch (light_.l[i].type) {
        case TYPE_POINT:
            l = vec3(light_.l[i].pos - v_.pos);
            dist = max(length(l), dist);
            l /= dist;
            atn = attenuation(dist, light_.l[i].range);
            break;

        case TYPE_SPOT:
            l = vec3(light_.l[i].pos - v_.pos);
            dist = max(length(l), dist);
            l /= dist;
            atn = attenuation(dist, light_.l[i].range) *
                  attenuation(light_.l[i].dir, l, light_.l[i].ang_scale,
                              light_.l[i].ang_off);
            break;

        case TYPE_DIRECT:
            l = -light_.l[i].dir;
            break;
        }

        float ndotl = max(dot(n, l), 0.0);

        if (ndotl == 0.0 && ndotv == 0.0)
            continue;

        vec3 h = normalize(l + v);
        float vdoth = max(dot(v, h), 0.0);
        float ndoth = max(dot(n, h), 0.0);

        vec3 fterm = fresnel_f(f0, f90, vdoth);
        vec3 diffuse = diffuse_brdf(albedo, fterm);
        vec3 specular = specular_brdf(fterm, ndotv, ndotl, ndoth, ar * ar);

        vec3 light = light_.l[i].clr * light_.l[i].inten * atn * ndotl;

        clr.rgb += (diffuse + specular) * light;
    }

    if (matl_.blend == BLEND_OPAQUE)
        clr.a = 1.0;

    return clr;
}

void main()
{
    clr_ = getclr();
}
