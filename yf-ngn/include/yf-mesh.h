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
 * Mesh file types.
 */
#define YF_FILETYPE_UNKNOWN  0
#define YF_FILETYPE_INTERNAL 1
#define YF_FILETYPE_GLTF     24

/**
 * Initializes a new mesh.
 *
 * @param filetype: The 'YF_FILETYPE' value indicating the format of the mesh
 *  file.
 * @param pathname: The pathname of the mesh file.
 * @param index: The index of the mesh to load.
 * @return: On success, returns a new mesh. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_mesh yf_mesh_init(int filetype, const char *pathname, size_t index);

/**
 * Deinitializes a mesh.
 *
 * @param mesh: The mesh to deinitialize. Can be 'NULL'.
 */
void yf_mesh_deinit(YF_mesh mesh);

YF_DECLS_END

#endif /* YF_YF_MESH_H */
