/*
 * YF
 * comp.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

layout(local_size_x=32) in;

layout(set=0, binding=0) buffer mbuffer {
    vec4 v[256];
} buf_;

void main()
{
}
