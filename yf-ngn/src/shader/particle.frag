/*
 * YF
 * particle.frag
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

/* TODO: Should be quads rather than points. */

/**
 * Primary texture.
 */
layout(set=1, binding=1) uniform sampler2D tex_;

layout(location=0) in iov {
    vec4 clr;
} v_;

layout(location=0) out vec4 clr_;

void main()
{
    clr_ = v_.clr * texture(tex_, gl_PointCoord);
}
