/*
 * YF
 * mdl16.vert.glsl
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
 * Type defining per-instance data.
 */
struct T_inst {
    mat4 m;
    mat4 mv;
};

/**
 * Global uniform data.
 */
layout(set=0, binding=0) uniform U_glob {
    mat4 v;
    mat4 p;
    mat4 o;
    T_vport vport[1];
} u_glob;

/**
 * Instance's uniform data.
 */
layout(set=1, binding=0) uniform U_inst {
    T_inst i[16];
} u_inst;

layout(location=0) in vec3 pos;
layout(location=1) in vec2 tc;
layout(location=2) in vec3 norm;

layout(location=0) out IO_vtx {
    vec2 tc;
    vec3 norm;
} out_vtx;

void main()
{
    gl_Position = u_glob.p * u_inst.i[gl_InstanceIndex].mv * vec4(pos, 1.0);
    out_vtx.tc = tc;
    out_vtx.norm = norm;
}
