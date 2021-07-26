/*
 * YF
 * mdl.vert.glsl
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#version 460 core

#ifndef VPORT_N
# error "VPORT_N not defined"
#endif

#ifndef INST_N
# error "INST_N not defined"
#endif

#ifndef JOINT_N
# error "JOINT_N not defined"
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
 * Type defining instance-specific data.
 */
struct T_inst {
    mat4 m;
    mat4 norm;
    mat4 mv;
    mat4 jnts[JOINT_N];
    mat4 jnts_norm[JOINT_N];
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
    T_inst i[INST_N];
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

/**
 * Gets vertex position.
 */
vec4 getpos()
{
    const int i = gl_InstanceIndex;

    mat4 skin = in_wgts.x * u_inst.i[i].jnts[in_jnts.x] +
                in_wgts.y * u_inst.i[i].jnts[in_jnts.y] +
                in_wgts.z * u_inst.i[i].jnts[in_jnts.z] +
                in_wgts.w * u_inst.i[i].jnts[in_jnts.w];

    return skin * vec4(in_pos, 1.0);
}

void main()
{
    const int i = gl_InstanceIndex;
    mat4 skin = in_wgts.x * u_inst.i[i].jnts[in_jnts.x] +
                in_wgts.y * u_inst.i[i].jnts[in_jnts.y] +
                in_wgts.z * u_inst.i[i].jnts[in_jnts.z] +
                in_wgts.w * u_inst.i[i].jnts[in_jnts.w];
    gl_Position = u_globl.p * u_inst.i[i].mv * skin * vec4(in_pos, 1.0);

    out_v.pos = in_pos; /* TODO */
    out_v.tc = in_tc;
    out_v.norm = in_norm; /* TODO */
    out_v.clr = in_clr;
}
