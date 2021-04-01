/*
 * YF
 * labl.vert.glsl
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
  float wdt;
  float hgt;
} u_inst;

layout(location=0) in vec3 pos;
layout(location=1) in vec2 tc;
layout(location=3) in vec4 clr;

out IO_vtx {
  layout(location=1) vec2 tc;
  layout(location=3) vec4 clr;
} out_vtx;

void main() {
  vec2 s;
  s = vec2(u_inst.wdt/u_glob.vport[0].wdt, u_inst.hgt/u_glob.vport[0].hgt);
  gl_Position = u_glob.o * u_inst.m * vec4(pos.xy*s, pos.z, 1.0);
  out_vtx.tc = tc;
  out_vtx.clr = clr;
}
