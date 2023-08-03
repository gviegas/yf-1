/*
 * YF
 * particle.vert
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

#extension GL_GOOGLE_include_directive : require

#include "shared.glsl"

/* TODO: Should be quads rather than points. */

/**
 * Instance's uniform data.
 */
layout(set=1, binding=0) uniform uinst {
    mat4 m;
    mat4 mv;
} inst_;

/* TODO: Other particle attributes. */
layout(location=0) in vec3 pos_;
layout(location=5) in vec4 clr_;

layout(location=0) out iov {
    vec4 clr;
} v_;

void main()
{
    /* TODO: Take this parameters from uniform buffer instead. */
    const float pt_min = 0.125;
    const float pt_max = 4.0;
    const float pt_fac = pt_max / (100.0 - 0.01);

    const float d = distance(globl_.v[3].xyz, pos_);
    gl_PointSize = clamp(abs(pt_max - d * pt_fac), pt_min, pt_max);

    /* TODO: MVP as uniform. */
    gl_Position = globl_.p * inst_.mv * vec4(pos_, 1.0);
    v_.clr = clr_;
}
