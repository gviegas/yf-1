/*
 * YF
 * model.vert.glsl
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
} globl_;

/**
 * Instance's uniform data.
 */
layout(set=1, binding=0) uniform U_inst {
    T_inst i[INST_N];
} inst_;

layout(location=0) in vec3 pos_;
layout(location=1) in vec2 tc_;
layout(location=2) in vec3 norm_;
layout(location=3) in vec4 tgnt_;
layout(location=4) in vec4 clr_;
layout(location=5) in uvec4 jnts_;
layout(location=6) in vec4 wgts_;

layout(location=0) out IO_v {
    vec3 pos;
    vec2 tc;
    vec3 norm;
    vec4 clr;
    vec3 eye;
} v_;

/**
 * Gets vertex position.
 */
vec4 getpos()
{
    const int i = gl_InstanceIndex;

    mat4 skin = wgts_.x * inst_.i[i].jnts[jnts_.x] +
                wgts_.y * inst_.i[i].jnts[jnts_.y] +
                wgts_.z * inst_.i[i].jnts[jnts_.z] +
                wgts_.w * inst_.i[i].jnts[jnts_.w];

    return skin * vec4(pos_, 1.0);
}

/**
 * Gets vertex normal.
 */
vec3 getnorm()
{
    const int i = gl_InstanceIndex;

    mat4 nskin = wgts_.x * inst_.i[i].jnts_norm[jnts_.x] +
                 wgts_.y * inst_.i[i].jnts_norm[jnts_.y] +
                 wgts_.z * inst_.i[i].jnts_norm[jnts_.z] +
                 wgts_.w * inst_.i[i].jnts_norm[jnts_.w];

    return normalize(mat3(nskin) * norm_);
}

void main()
{
    const int i = gl_InstanceIndex;

    vec4 pos = inst_.i[i].m * getpos();
    vec3 norm = normalize(vec3(inst_.i[i].norm * vec4(getnorm(), 0.0)));

    gl_Position = globl_.vp * pos;

    v_.pos = pos.xyz / pos.w;
    v_.tc = tc_;
    v_.norm = norm;
    v_.clr = clr_;
    v_.eye = globl_.v[3].xyz - pos.xyz;
}
