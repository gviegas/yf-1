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
layout(set=1, binding=4) uniform sampler2D tex_;

layout(location=0) in IO_v {
    vec3 pos;
    vec2 tc;
    vec3 norm;
    vec4 clr;
} v_;

layout(location=0) out vec4 clr_;

void main()
{
    clr_ = textureLod(tex_, v_.tc, 0.0);
}
