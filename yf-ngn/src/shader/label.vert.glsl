/*
 * YF
 * label.vert.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

#ifndef VPORT_N
# error "VPORT_N not defined"
#endif

layout(std140, column_major) uniform;

/**
 * Type defining a viewport.
 */
struct T_vport {
    float x;
    float y;
    float wdt;
    float hgt;
    float near;
    float far;
};

/**
 * Global uniform data.
 */
layout(set=0, binding=0) uniform U_globl {
    mat4 v;
    mat4 p;
    mat4 o;
    mat4 vp;
    T_vport vport[VPORT_N];
} globl_;

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
layout(location=1) in vec2 tc_;
layout(location=4) in vec4 clr_;

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
