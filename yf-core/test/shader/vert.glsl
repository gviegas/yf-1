/*
 * YF
 * vert.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

layout(set=0, binding=0) uniform U_buf {
    mat4 m;
} buf_;

layout(location=0) in vec3 pos_;
layout(location=1) in vec4 clr_;

layout(location=0) out IO_v {
    vec4 clr;
} v_;

void main()
{
    gl_Position = buf_.m * vec4(pos_, 1.0);
    v_.clr = clr_;
}
