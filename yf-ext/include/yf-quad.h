/*
 * YF
 * yf-quad.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_QUAD_H
#define YF_YF_QUAD_H

#include <yf/com/yf-defs.h>

#include "yf-node.h"
#include "yf-matrix.h"
#include "yf-mesh.h"
#include "yf-texture.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a drawable 2D quad.
 */
typedef struct YF_quad_o *YF_quad;

/**
 * Initializes a new quad.
 *
 * @return: On success, returns a new quad. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_quad yf_quad_init(void);

/**
 * Gets the node of a quad.
 *
 * @param quad: The quad.
 * @return: The quad's node.
 */
YF_node yf_quad_getnode(YF_quad quad);

/**
 * Gets the transformation matrix of a quad.
 *
 * @param quad: The quad.
 * @return: The quad's transformation matrix.
 */
YF_mat4 *yf_quad_getxform(YF_quad quad);

/**
 * Gets the mesh of a quad.
 *
 * @param quad: The quad.
 * @return: The mesh used by the quad.
 */
YF_mesh yf_quad_getmesh(YF_quad quad);

/**
 * Gets the texture of a quad.
 *
 * @param quad: The quad.
 * @return: The texture used by the quad, or 'NULL' if none is set.
 */
YF_texture yf_quad_gettex(YF_quad quad);

/**
 * Sets the texture for a quad.
 *
 * @param quad: The quad.
 * @param tex: The texture to set. Can be 'NULL'.
 */
void yf_quad_settex(YF_quad quad, YF_texture tex);

/**
 * Deinitializes a quad.
 *
 * @param quad: The quad to deinitialize. Can be 'NULL'.
 */
void yf_quad_deinit(YF_quad quad);

YF_DECLS_END

#endif /* YF_YF_QUAD_H */
