/*
 * YF
 * yf-mesh.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_MESH_H
#define YF_YF_MESH_H

#include <stddef.h>

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
 * Vertex attribute semantics.
 */
#define YF_VSEMT_POS  0x01
#define YF_VSEMT_NORM 0x02
#define YF_VSEMT_TGNT 0x04
#define YF_VSEMT_TC   0x08
#define YF_VSEMT_TC1  0x10
#define YF_VSEMT_CLR  0x20
#define YF_VSEMT_JNTS 0x40
#define YF_VSEMT_WGTS 0x80

/**
 * Type defining vertex attribute data.
 */
typedef struct {
    int vsemt;
    int vfmt;
    /* Offset into 'meshdt.data', relative to 'primdt.data_off'. */
    size_t data_off;
} YF_attrdt;

/**
 * Type defining primitive data.
 */
typedef struct {
    int topology;
    unsigned vert_n;
    unsigned indx_n;
    /* Offset into 'meshdt.data'. */
    size_t data_off;

    unsigned vsemt_mask;
    YF_attrdt *attrs;
    unsigned attr_n;

    int itype;
    /* Offset into 'meshdt.data', relative to 'primdt.data_off'. */
    size_t indx_data_off;

    /* XXX: Does not take ownership. */
    YF_material matl;
} YF_primdt;

/**
 * Type defining mesh data.
 */
typedef struct {
    YF_primdt *prims;
    unsigned prim_n;
    void *data;
    size_t data_sz;
} YF_meshdt;

/**
 * Initializes a new mesh.
 *
 * @param data: The data from which to initialize the mesh.
 * @return: On success, returns a new mesh. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_mesh yf_mesh_init(const YF_meshdt *data);

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
