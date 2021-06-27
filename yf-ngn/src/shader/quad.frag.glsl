/*
 * YF
 * quad.frag.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

/**
 * Primary texture.
 */
layout(set=1, binding=4) uniform sampler2D u_tex;

layout(location=0) in IO_v {
    vec2 tc;
    vec4 clr;
} in_v;

layout(location=0) out vec4 out_clr;

void main()
{
    out_clr = in_v.clr * textureLod(u_tex, in_v.tc, 0.0);
}
