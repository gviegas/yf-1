/*
 * YF
 * terr.frag.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

/**
 * Primary texture.
 */
layout(set=1, binding=4) uniform sampler2D tex_;

layout(location=0) in IO_v {
    vec2 tc;
    vec3 norm;
} v_;

layout(location=0) out vec4 clr_;

void main()
{
    clr_ = textureLod(tex_, v_.tc, 0.0);
}
