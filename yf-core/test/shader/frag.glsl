/*
 * YF
 * frag.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

layout(location=0) in vertex {
    vec4 color;
} v_;

layout(location=0) out vec4 frag0_;

void main()
{
    frag0_ = v_.color;
}
