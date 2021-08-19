/*
 * YF
 * part.frag.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

/**
 * Primary texture.
 */
layout(set=1, binding=4) uniform sampler2D tex_;

layout(location=0) in IO_v {
    vec4 clr;
} v_;

layout(location=0) out vec4 clr_;

void main()
{
    clr_ = v_.clr * textureLod(tex_, gl_PointCoord, 0.0);
}
