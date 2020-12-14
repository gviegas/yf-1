/*
 * YF
 * yf-mesh.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_MESH_H
#define YF_YF_MESH_H

#include <yf/com/yf-defs.h>

YF_DECLS_BEGIN

/* Opaque type defining the mesh data. */
typedef struct YF_mesh_o *YF_mesh;

/* Initializes a new mesh. */
YF_mesh yf_mesh_init(int filetype, const char *pathname);

/* Deinitializes a mesh. */
void yf_mesh_deinit(YF_mesh mesh);

YF_DECLS_END

#endif /* YF_YF_MESH_H */
