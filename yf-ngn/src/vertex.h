/*
 * YF
 * vertex.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_VERTEX_H
#define YF_VERTEX_H

#include "yf-vector.h"

/* Type defining the mesh vertex for model objects. */
typedef struct {
    YF_vec3 pos;
    YF_vec2 tc;
    YF_vec3 norm;
    /* TODO */
} YF_vmdl;
#define YF_VTYPE_MDL 0

/* Type defining the mesh vertex for terrain objects. */
typedef struct {
    YF_vec3 pos;
    YF_vec2 tc;
    YF_vec3 norm;
    /* TODO */
} YF_vterr;
#define YF_VTYPE_TERR 1

/* Type defining the mesh vertex for particle system objects. */
typedef struct {
    YF_vec3 pos;
    YF_vec4 clr;
    /* TODO */
} YF_vpart;
#define YF_VTYPE_PART 2

/* Type defining the mesh vertex for quad objects. */
typedef struct {
    YF_vec3 pos;
    YF_vec2 tc;
    YF_vec4 clr;
    /* TODO */
} YF_vquad;
#define YF_VTYPE_QUAD 3

/* Type defining the mesh vertex for label objects. */
typedef struct {
    YF_vec3 pos;
    YF_vec2 tc;
    YF_vec4 clr;
    /* TODO */
} YF_vlabl;
#define YF_VTYPE_LABL 4

#endif /* YF_VERTEX_H */
