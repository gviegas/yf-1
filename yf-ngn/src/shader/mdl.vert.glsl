/*
 * YF
 * mdl.vert.glsl
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#version 460 core

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
    T_vport vport[1];
} u_globl;

/**
 * Instance's uniform data.
 */
layout(set=1, binding=0) uniform U_inst {
    mat4 m;
    mat4 nm;
    mat4 mv;
} u_inst;

layout(location=0) in vec3 in_pos;
layout(location=1) in vec2 in_tc;
layout(location=2) in vec3 in_norm;
layout(location=3) in vec4 in_tgnt;
layout(location=4) in vec4 in_clr;
layout(location=5) in uvec4 in_jnts;
layout(location=6) in vec4 in_wgts;

layout(location=0) out IO_v {
    vec3 pos;
    vec2 tc;
    vec3 norm;
    vec4 clr;
} out_v;

void main()
{
    gl_Position = u_globl.p * u_inst.mv * vec4(in_pos, 1.0);
    out_v.pos = in_pos; /* TODO */
    out_v.tc = in_tc;
    out_v.norm = in_norm; /* TODO */
    out_v.clr = in_clr;
}
