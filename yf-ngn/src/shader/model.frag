/*
 * YF
 * model.frag
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

#extension GL_GOOGLE_include_directive : require

#include "light.glsl"
#include "material.glsl"

layout(location=0) in IO_v {
    vec3 pos;
    vec3 norm;
    vec2 tc;
    vec4 clr;
    vec3 eye;
} v_;

layout(location=0) out vec4 clr_;

/**
 * Gets fragment color.
 */
vec4 getclr()
{
    /* TODO: Normal/occlusion/emissive maps. */

    vec4 clr = v_.clr;

    if ((matl_.tex_mask & TEX_CLR) == TEX_CLR)
        clr *= texture(clr_is_, v_.tc);
    clr *= matl_.clr_fac;

    vec3 albedo, f0, f90;
    float ar;

    switch (matl_.method) {
    case METHOD_PBRSG:
        vec3 specular = matl_.pbr_fac.rgb;
        float glossiness = matl_.pbr_fac.a;
        if ((matl_.tex_mask & TEX_PBR) == TEX_PBR) {
            vec4 spec_gloss = texture(pbr_is_, v_.tc);
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
            vec4 metal_rough = texture(pbr_is_, v_.tc);
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
