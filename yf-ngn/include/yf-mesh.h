/*
 * YF
 * yf-mesh.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_MESH_H
#define YF_YF_MESH_H

#include "yf/com/yf-defs.h"

#include "yf-collection.h"
#include "yf-material.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a mesh.
 */
typedef struct YF_mesh_o *YF_mesh;

/**
 * Loads a new mesh from file.
 *
 * @param pathname: The pathname of the mesh file.
 * @param index: The index of the mesh to load.
 * @param coll: The collection for the new mesh.
 * @return: On success, returns a new mesh. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_mesh yf_mesh_load(const char *pathname, size_t index, YF_collection coll);

/**
 * Gets the number of primitives of a mesh.
 *
 * @param mesh: The mesh.
 * @return: The number of primitives of 'mesh'. This value corresponds to the
 *  number of draw calls required to render the mesh.
 */
unsigned yf_mesh_getprimn(YF_mesh mesh);

/**
 * Gets the material of a mesh's primitive.
 *
 * @param mesh: The mesh.
 * @param prim: The primitive's index.
 * @return: The material used by the given primitive.
 */
YF_material yf_mesh_getmatl(YF_mesh mesh, unsigned prim);

/**
 * Sets the material for a mesh's primitive.
 *
 * @param mesh: The mesh.
 * @param prim: The primitive's index.
 * @param matl: The material to set. Can be 'NULL'.
 * @return: The replaced material.
 */
YF_material yf_mesh_setmatl(YF_mesh mesh, unsigned prim, YF_material matl);

/**
 * Deinitializes a mesh.
 *
 * @param mesh: The mesh to deinitialize. Can be 'NULL'.
 */
void yf_mesh_deinit(YF_mesh mesh);

YF_DECLS_END

#endif /* YF_YF_MESH_H */
