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
struct vport {
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
layout(set=0, binding=0) uniform uglobl {
    mat4 v;
    mat4 p;
    mat4 o;
    mat4 vp;
    vport vport[VPORT_N];
} globl_;
