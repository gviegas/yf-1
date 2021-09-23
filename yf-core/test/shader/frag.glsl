/*
 * YF
 * frag.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

layout(location=0) in IO_v {
    vec4 clr;
} v_;

layout(location=0) out vec4 clr_;

void main()
{
    clr_ = v_.clr;
}
