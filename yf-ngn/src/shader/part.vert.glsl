/*
 * YF
 * part.vert.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
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
    mat4 m;
    mat4 mv;
} u_inst;

layout(location=0) in vec3 pos;
layout(location=3) in vec4 clr;

layout(location=0) out IO_vtx {
    vec4 clr;
} out_vtx;

void main()
{
    /* TODO: Take this parameters from uniform buffer instead. */
    const float pt_min = 0.125;
    const float pt_max = 4.0;
    const float pt_fac = pt_max / (100.0-0.01);

    const float d = distance(u_glob.v[3].xyz, pos);
    gl_PointSize = clamp(abs(pt_max-d*pt_fac), pt_min, pt_max);

    gl_Position = u_glob.p * u_inst.mv * vec4(pos, 1.0);
    out_vtx.clr = clr;
}
