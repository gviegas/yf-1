/*
 * YF
 * shared.glsl
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef VPORT_N
# error "VPORT_N not defined"
#endif

layout(std140, column_major) uniform;

/**
 * Viewport.
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
