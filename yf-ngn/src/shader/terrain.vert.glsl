/*
 * YF
 * terrain.vert.glsl
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
} inst_;

/**
 * Height map.
 */
layout(set=1, binding=2) uniform sampler2D hmap_;

layout(location=0) in vec3 pos_;
layout(location=1) in vec2 tc_;
layout(location=2) in vec3 norm_;

layout(location=0) out IO_v {
    vec2 tc;
    vec3 norm;
} v_;

void main()
{
    const float y = textureLod(hmap_, tc_, 0.0).r;
    gl_Position = globl_.p * inst_.mv * vec4(pos_.x, y, pos_.z, 1.0);

    v_.tc = tc_;
    v_.norm = norm_;
}
