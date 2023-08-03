/*
 * YF
 * mesh.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_MESH_H
#define YF_MESH_H

#include "yf/core/yf-cmdbuf.h"

#include "yf-mesh.h"

/* Replaces the contents of a mesh object's data buffer. */
int yf_mesh_setdata(yf_mesh_t *mesh, size_t offset, const void *data,
                    size_t size);

/* Encodes vertex/index state and draws a given mesh. */
void yf_mesh_draw(yf_mesh_t *mesh, yf_cmdbuf_t *cmdb, unsigned inst_n);

#endif /* YF_MESH_H */
