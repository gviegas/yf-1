/*
 * YF
 * yf-terrain.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_TERRAIN_H
#define YF_YF_TERRAIN_H

#include <yf/com/yf-defs.h>

#include "yf-node.h"
#include "yf-matrix.h"
#include "yf-mesh.h"
#include "yf-texture.h"

YF_DECLS_BEGIN

/* Opaque type defining drawable terrain. */
typedef struct YF_terrain_o *YF_terrain;

/* Initializes a new terrain. */
YF_terrain yf_terrain_init(unsigned width, unsigned depth);

/* Gets the node of a terrain. */
YF_node yf_terrain_getnode(YF_terrain terr);

/* Gets the transformation matrix of a terrain. */
YF_mat4 *yf_terrain_getxform(YF_terrain terr);

/* Gets the mesh of a terrain. */
YF_mesh yf_terrain_getmesh(YF_terrain terr);

/* Gets the height map of a terrain. */
YF_texture yf_terrain_gethmap(YF_terrain terr);

/* Sets the height map for a terrain. */
void yf_terrain_sethmap(YF_terrain terr, YF_texture hmap);

/* Gets the texture of a terrain. */
YF_texture yf_terrain_gettex(YF_terrain terr);

/* Sets the texture for a terrain. */
void yf_terrain_settex(YF_terrain terr, YF_texture tex);

/* Deinitializes a terrain. */
void yf_terrain_deinit(YF_terrain terr);

YF_DECLS_END

#endif /* YF_YF_TERRAIN_H */
