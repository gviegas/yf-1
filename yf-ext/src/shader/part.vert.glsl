/*
 * YF
 * part.vert.glsl
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

layout(location=0) in vec3 pos;
layout(location=3) in vec4 clr;

out IO_vtx {
  layout(location=3) vec4 clr;
} out_vtx;

void main() {
  /* TODO: Take this parameters from uniform buffer instead. */
  const float z_dist = 100.0;
  const float pt_min = 0.125;
  const float pt_max = 10.0;

  const float d = distance(pos, u_inst.mvp[3].xyz);
  gl_PointSize = clamp(0.1 * (z_dist-d), pt_min, pt_max);

  gl_Position = u_inst.mvp * vec4(pos, 1.0);
  out_vtx.clr = clr;
}
