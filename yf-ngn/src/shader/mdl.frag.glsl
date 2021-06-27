/*
 * YF
 * mdl.frag.glsl
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#version 460 core

/**
 * Primary texture.
 */
layout(set=1, binding=4) uniform sampler2D u_tex;

layout(location=0) in IO_v {
    vec3 pos;
    vec2 tc;
    vec3 norm;
    vec4 clr;
} in_v;

layout(location=0) out vec4 out_clr;

void main()
{
    out_clr = textureLod(u_tex, in_v.tc, 0.0);
}
