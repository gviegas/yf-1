/*
 * YF
 * part.frag.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#version 460 core

/**
 * Primary texture.
 */
layout(set=1, binding=1) uniform sampler2D u_tex;

in IO_vtx {
  layout(location=3) vec4 clr;
} in_vtx;

layout(location=0) out vec4 clr0;

void main() {
  clr0 = in_vtx.clr * textureLod(u_tex, gl_PointCoord, 0.0);
}
