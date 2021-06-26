/*
 * YF
 * quad.vert.glsl
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
} u_globl;

/**
 * Instance's uniform data.
 */
layout(set=1, binding=0) uniform U_inst {
    mat4 m;
    mat4 mv;
    float wdt;
    float hgt;
} u_inst;

layout(location=0) in vec3 in_pos;
layout(location=1) in vec2 in_tc;
layout(location=2) in vec4 in_clr;

layout(location=0) out IO_v {
    vec2 tc;
    vec4 clr;
} out_v;

void main()
{
    vec2 s = vec2(u_inst.wdt / u_globl.vport[0].wdt,
                  u_inst.hgt / u_globl.vport[0].hgt);
    gl_Position = u_globl.o * u_inst.m * vec4(in_pos.xy * s, in_pos.z, 1.0);

    out_v.tc = in_tc;
    out_v.clr = in_clr;
}
