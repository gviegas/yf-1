/*
 * YF
 * yf-mesh.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_MESH_H
#define YF_YF_MESH_H

#include <stddef.h>

#include "yf/com/yf-defs.h"

#include "yf-collec.h"
#include "yf-material.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a mesh.
 */
typedef struct yf_mesh yf_mesh_t;

/**
 * Loads a new mesh from file.
 *
 * @param pathname: The pathname of the mesh file.
 * @param index: The index of the mesh to load.
 * @param coll: The collection for the new mesh.
 * @return: On success, returns a new mesh. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
yf_mesh_t *yf_mesh_load(const char *pathname, size_t index, yf_collec_t *coll);

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
typedef struct yf_attrdt {
    int vsemt;
    int vfmt;
    /* Offset into 'meshdt.data', relative to 'primdt.data_off'. */
    size_t data_off;
} yf_attrdt_t;

/**
 * Type defining primitive data.
 */
typedef struct yf_primdt {
    int topology;
    unsigned vert_n;
    unsigned indx_n;
    /* Offset into 'meshdt.data'. */
    size_t data_off;

    unsigned vsemt_mask;
    yf_attrdt_t *attrs;
    unsigned attr_n;

    int itype;
    /* Offset into 'meshdt.data', relative to 'primdt.data_off'. */
    size_t indx_data_off;

    /* XXX: Does not take ownership. */
    yf_material_t *matl;
} yf_primdt_t;

/**
 * Type defining mesh data.
 */
typedef struct yf_meshdt {
    yf_primdt_t *prims;
    unsigned prim_n;
    void *data;
    size_t data_sz;
} yf_meshdt_t;

/**
 * Initializes a new mesh.
 *
 * @param data: The data from which to initialize the mesh.
 * @return: On success, returns a new mesh. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
yf_mesh_t *yf_mesh_init(const yf_meshdt_t *data);

/**
 * Gets the number of primitives of a mesh.
 *
 * @param mesh: The mesh.
 * @return: The number of primitives of 'mesh'. This value corresponds to the
 *  number of draw calls required to render the mesh.
 */
unsigned yf_mesh_getprimn(yf_mesh_t *mesh);

/**
 * Gets the material of a mesh's primitive.
 *
 * @param mesh: The mesh.
 * @param prim: The primitive's index.
 * @return: The material used by the given primitive.
 */
yf_material_t *yf_mesh_getmatl(yf_mesh_t *mesh, unsigned prim);

/**
 * Sets the material for a mesh's primitive.
 *
 * @param mesh: The mesh.
 * @param prim: The primitive's index.
 * @param matl: The material to set. Can be 'NULL'.
 * @return: The replaced material.
 */
yf_material_t *yf_mesh_setmatl(yf_mesh_t *mesh, unsigned prim,
                               yf_material_t *matl);

/**
 * Deinitializes a mesh.
 *
 * @param mesh: The mesh to deinitialize. Can be 'NULL'.
 */
void yf_mesh_deinit(yf_mesh_t *mesh);

YF_DECLS_END

#endif /* YF_YF_MESH_H */
