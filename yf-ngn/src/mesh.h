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
void yf_mesh_draw(YF_mesh mesh, YF_cmdbuf cmdb, unsigned inst_n, int inst_id);

#endif /* YF_MESH_H */
