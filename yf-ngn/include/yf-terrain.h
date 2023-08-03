/*
 * YF
 * yf-terrain.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_TERRAIN_H
#define YF_YF_TERRAIN_H

#include "yf/com/yf-defs.h"

#include "yf-node.h"
#include "yf-mesh.h"
#include "yf-texture.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining drawable terrain.
 */
typedef struct yf_terrain yf_terrain_t;

/**
 * Initializes a new terrain.
 *
 * @param width: The width of the terrain, in grid cells.
 * @param depth: The depth of the terrain, in grid cells.
 * @return: On success, returns a new terrain. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_terrain_t *yf_terrain_init(unsigned width, unsigned depth);

/**
 * Gets the node of a terrain.
 *
 * @param terr: The terrain.
 * @return: The terrain's node.
 */
yf_node_t *yf_terrain_getnode(yf_terrain_t *terr);

/**
 * Gets the mesh of a terrain.
 *
 * @param terr: The terrain.
 * @return: The mesh used by the terrain.
 */
yf_mesh_t *yf_terrain_getmesh(yf_terrain_t *terr);

/**
 * Gets the height map of a terrain.
 *
 * @param terr: The terrain.
 * @return: The height map used by the terrain, or 'NULL' if none is set.
 */
yf_texture_t *yf_terrain_gethmap(yf_terrain_t *terr);

/**
 * Sets the height map for a terrain.
 *
 * @param terr: The terrain.
 * @param hmap: The height map to set. Can be 'NULL'.
 */
void yf_terrain_sethmap(yf_terrain_t *terr, yf_texture_t *hmap);

/**
 * Gets the texture of a terrain.
 *
 * @param terr: The terrain.
 * @return: The texture used by the terrain, or 'NULL' if none is set.
 */
yf_texture_t *yf_terrain_gettex(yf_terrain_t *terr);

/**
 * Sets the texture for a terrain.
 *
 * @param terr: The terrain.
 * @param tex: The texture to set. Can be 'NULL'.
 */
void yf_terrain_settex(yf_terrain_t *terr, yf_texture_t *tex);

/**
 * Deinitializes a terrain.
 *
 * @param terr: The terrain to deinitialize. Can be 'NULL'.
 */
void yf_terrain_deinit(yf_terrain_t *terr);

YF_DECLS_END

#endif /* YF_YF_TERRAIN_H */
