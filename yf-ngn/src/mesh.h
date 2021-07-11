/*
 * YF
 * mesh.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_MESH_H
#define YF_MESH_H

#include "yf/core/yf-cmdbuf.h"

#include "yf-mesh.h"
#include "yf-vector.h"

/* Type defining the vertex of model meshes. */
#define YF_VTYPE_MDL 0
typedef struct {
    YF_vec3 pos;
    YF_vec2 tc;
    YF_vec3 norm;
    /* TODO */
} YF_vmdl;

/* Type defining the vertex of terrain meshes. */
#define YF_VTYPE_TERR 1
typedef struct {
    YF_vec3 pos;
    YF_vec2 tc;
    YF_vec3 norm;
    /* TODO */
} YF_vterr;

/* Type defining the vertex of particle system meshes. */
#define YF_VTYPE_PART 2
typedef struct {
    YF_vec3 pos;
    YF_vec4 clr;
    /* TODO */
} YF_vpart;

/* Type defining the vertex of quad meshes. */
#define YF_VTYPE_QUAD 3
typedef struct {
    YF_vec3 pos;
    YF_vec2 tc;
    YF_vec4 clr;
    /* TODO */
} YF_vquad;

/* Type defining the vertex of label meshes. */
#define YF_VTYPE_LABL 4
typedef struct {
    YF_vec3 pos;
    YF_vec2 tc;
    YF_vec4 clr;
    /* TODO */
} YF_vlabl;

/* Type defining the mesh data. */
typedef struct {
    struct {
        int vtype;
        void *data;
        size_t n;
    } v;
    struct {
        void *data;
        short stride;
        size_t n;
    } i;
} YF_meshdt;

/* Initializes a new mesh object from mesh data directly. */
YF_mesh yf_mesh_initdt(const YF_meshdt *data);

/* Replaces the contents of a mesh object's vertices. */
int yf_mesh_setvtx(YF_mesh mesh, YF_slice range, const void *data);

/* Encodes vertex/index state and draws a given mesh. */
void yf_mesh_draw(YF_mesh mesh, YF_cmdbuf cmdb, unsigned inst_n);

#endif /* YF_MESH_H */
