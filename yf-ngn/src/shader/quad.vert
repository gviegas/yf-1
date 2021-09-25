/*
 * YF
 * quad.vert
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

#extension GL_GOOGLE_include_directive : require

#include "shared.glsl"

/**
 * Instance's uniform data.
 */
layout(set=1, binding=0) uniform U_inst {
    mat4 m;
    mat4 mv;
    float wdt;
    float hgt;
} inst_;

layout(location=0) in vec3 pos_;
layout(location=3) in vec2 tc_;
layout(location=5) in vec4 clr_;

layout(location=0) out IO_v {
    vec2 tc;
    vec4 clr;
} v_;

void main()
{
    vec2 s = vec2(inst_.wdt / globl_.vport[0].wdt,
                  inst_.hgt / globl_.vport[0].hgt);

    gl_Position = globl_.o * inst_.m * vec4(pos_.xy * s, pos_.z, 1.0);

    v_.tc = tc_;
    v_.clr = clr_;
}
