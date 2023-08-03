/*
 * YF
 * terrain.frag
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

/**
 * Primary texture.
 */
layout(set=1, binding=1) uniform sampler2D tex_;

layout(location=0) in iov {
    vec3 norm;
    vec2 tc;
} v_;

layout(location=0) out vec4 clr_;

void main()
{
    clr_ = texture(tex_, v_.tc);
}
