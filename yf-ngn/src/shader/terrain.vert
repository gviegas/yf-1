/*
 * YF
 * terrain.vert
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

#extension GL_GOOGLE_include_directive : require

#include "shared.glsl"

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
    const float y = texture(hmap_, tc_).r;
    gl_Position = globl_.p * inst_.mv * vec4(pos_.x, y, pos_.z, 1.0);

    v_.tc = tc_;
    v_.norm = norm_;
}
