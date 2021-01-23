/*
 * YF
 * terr.vert.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

layout(set=0, binding=0) uniform U_glob {
  mat4 v;
  mat4 p;
} u_glob;

layout(set=1, binding=0) uniform U_inst {
  mat4 m;
  mat4 mvp;
} u_inst;

layout(set=1, binding=3) uniform sampler2D u_hmap;

layout(location=0) in vec3 pos;
layout(location=1) in vec2 tc;
layout(location=2) in vec3 norm;

out IO_vtx {
  layout(location=1) vec2 tc;
  layout(location=2) vec3 norm;
} out_vtx;

void main() {
  float y = -textureLod(u_hmap, tc, 0.0).r;
  gl_Position = u_inst.mvp * vec4(pos.x, y, pos.z, 1.0);
  out_vtx.tc = tc;
  out_vtx.norm = norm;
}
