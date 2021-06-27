/*
 * YF
 * terr.vert.glsl
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
} u_inst;

/**
 * Height map.
 */
layout(set=1, binding=2) uniform sampler2D u_hmap;

layout(location=0) in vec3 in_pos;
layout(location=1) in vec2 in_tc;
layout(location=2) in vec3 in_norm;

layout(location=0) out IO_v {
    vec2 tc;
    vec3 norm;
} out_v;

void main()
{
    const float y = textureLod(u_hmap, in_tc, 0.0).r;
    gl_Position = u_globl.p * u_inst.mv * vec4(in_pos.x, y, in_pos.z, 1.0);

    out_v.tc = in_tc;
    out_v.norm = in_norm;
}
