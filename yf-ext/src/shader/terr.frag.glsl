/*
 * YF
 * terr.frag.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

layout(set=1, binding=1) uniform sampler2D u_tex;

in IO_vtx {
  layout(location=1) vec2 tc;
  layout(location=2) vec3 norm;
} in_vtx;

layout(location=0) out vec4 clr0;

void main() {
  clr0 = textureLod(u_tex, in_vtx.tc, 0.0);
}
