/*
 * YF
 * part.vert.glsl
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

/* TODO: Other per-particle params., e.g. clrN, size... */
layout(location=0) in vec3 in_pos;
layout(location=4) in vec4 in_clr;

layout(location=0) out IO_v {
    vec4 clr;
} out_v;

void main()
{
    /* TODO: Take this parameters from uniform buffer instead. */
    const float pt_min = 0.125;
    const float pt_max = 4.0;
    const float pt_fac = pt_max / (100.0 - 0.01);

    const float d = distance(u_globl.v[3].xyz, in_pos);
    gl_PointSize = clamp(abs(pt_max - d * pt_fac), pt_min, pt_max);

    /* TODO: MVP as uniform. */
    gl_Position = u_globl.p * u_inst.mv * vec4(in_pos, 1.0);
    out_v.clr = in_clr;
}
