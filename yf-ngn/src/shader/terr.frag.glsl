/*
 * YF
 * terr.frag.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

/**
 * Primary texture.
 */
layout(set=1, binding=1) uniform sampler2D u_tex;

layout(location=0) in IO_vtx {
  vec2 tc;
  vec3 norm;
} in_vtx;

layout(location=0) out vec4 clr0;

void main()
{
  clr0 = textureLod(u_tex, in_vtx.tc, 0.0);
}
