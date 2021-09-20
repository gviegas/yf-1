/*
 * YF
 * yf-mesh.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_MESH_H
#define YF_YF_MESH_H

#include "yf/com/yf-defs.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a mesh.
 */
typedef struct YF_mesh_o *YF_mesh;

/**
 * Initializes a new mesh.
 *
 * @param pathname: The pathname of the mesh file.
 * @param index: The index of the mesh to load.
 * @return: On success, returns a new mesh. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_mesh yf_mesh_init(const char *pathname, size_t index);

/**
 * Gets the number of primitives of a mesh.
 *
 * @param mesh: The mesh.
 * @return: The number of primitives of 'mesh'. This value corresponds to the
 *  number of draw calls required to render the mesh.
 */
unsigned yf_mesh_getprimn(YF_mesh mesh);

/**
 * Deinitializes a mesh.
 *
 * @param mesh: The mesh to deinitialize. Can be 'NULL'.
 */
void yf_mesh_deinit(YF_mesh mesh);

YF_DECLS_END

#endif /* YF_YF_MESH_H */
