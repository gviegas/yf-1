/*
 * YF
 * model.frag.glsl
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#version 460 core

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

/* TODO: Punctual lights. */
#define LIGHT_DIR    vec3(-0.7527726527, -0.6199304199, -0.2214037214)
#define LIGHT_INTENS vec3(6.0)
#define LIGHT_CLR    vec3(1.0)

layout(std140, column_major) uniform;

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
    /* TODO: Vertex color; normal/occlusion/emissive maps. */

    vec4 clr = vec4(1.0);

    if ((matl_.tex_mask & TEX_CLR) == TEX_CLR)
        clr = textureLod(clr_is_, v_.tc, 0.0);
    clr *= matl_.clr_fac;

    if (matl_.method == METHOD_UNLIT) {
        if (matl_.blend == BLEND_OPAQUE)
            clr.a = 1.0;

        return clr;
    }

    vec3 v = normalize(v_.eye);
    vec3 l = normalize(-LIGHT_DIR);
    vec3 n = normalize(v_.norm);
    vec3 h = normalize(l + v);
    float vdoth = max(dot(v, h), 0.0);
    float ndotv = max(dot(n, v), 0.0);
    float ndotl = max(dot(n, l), 0.0);
    float ndoth = max(dot(n, h), 0.0);

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
    }

    vec3 fterm = fresnel_f(f0, f90, vdoth);
    vec3 diffuse = diffuse_brdf(albedo, fterm);
    vec3 specular = specular_brdf(fterm, ndotv, ndotl, ndoth, ar*ar);
    clr.xyz = (diffuse + specular) * LIGHT_INTENS * LIGHT_CLR * ndotl;

    if (matl_.blend == BLEND_OPAQUE)
        clr.a = 1.0;

    return clr;
}

void main()
{
    clr_ = getclr();
}
