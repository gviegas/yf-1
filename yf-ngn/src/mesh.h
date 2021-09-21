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

/* Type defining attribute data. */
typedef struct {
    unsigned loc;
    int vfmt;
    /* relative to 'primdt.data_off' */
    size_t data_off;
} YF_attrdt;

/* Type defining primitive data. */
typedef struct {
    int topology;
    unsigned vert_n;
    unsigned indx_n;
    /* relative to 'meshdt.data' */
    size_t data_off;

    YF_attrdt *attrs;
    unsigned attr_n;

    int itype;
    /* relative to 'primdt.data_off' */
    size_t indx_data_off;

    /* XXX: Does not take ownership. */
    YF_material matl;
} YF_primdt;

/* Type defining mesh data. */
typedef struct {
    YF_primdt *prims;
    unsigned prim_n;
    void *data;
    size_t data_sz;
} YF_meshdt;

/* Initializes a new mesh object from mesh data directly. */
YF_mesh yf_mesh_initdt(const YF_meshdt *data);

/* Replaces the contents of a mesh object's data buffer. */
int yf_mesh_setdata(YF_mesh mesh, size_t offset, const void *data, size_t size);

/* Encodes vertex/index state and draws a given mesh. */
void yf_mesh_draw(YF_mesh mesh, YF_cmdbuf cmdb, unsigned inst_n);

#endif /* YF_MESH_H */
