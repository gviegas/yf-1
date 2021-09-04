/*
 * YF
 * mdl.frag.glsl
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

layout(std140, column_major) uniform;

/**
 * Material.
 */
layout(set=1, binding=3) uniform U_matl {
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
layout(set=1, binding=4) uniform sampler2D clr_is_;
layout(set=1, binding=5) uniform sampler2D pbr_is_;
layout(set=1, binding=6) uniform sampler2D norm_is_;
layout(set=1, binding=7) uniform sampler2D occ_is_;
layout(set=1, binding=8) uniform sampler2D emis_is_;

layout(location=0) in IO_v {
    vec3 pos;
    vec2 tc;
    vec3 norm;
    vec4 clr;
} v_;

layout(location=0) out vec4 clr_;

void main()
{
    clr_ = textureLod(clr_is_, v_.tc, 0.0);
}
