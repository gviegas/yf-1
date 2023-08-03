/*
 * YF
 * yf-quad.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_QUAD_H
#define YF_YF_QUAD_H

#include "yf/com/yf-defs.h"

#include "yf-node.h"
#include "yf-mesh.h"
#include "yf-texture.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a drawable 2D quad.
 */
typedef struct yf_quad yf_quad_t;

/**
 * Initializes a new quad.
 *
 * @return: On success, returns a new quad. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
yf_quad_t *yf_quad_init(void);

/**
 * Gets the node of a quad.
 *
 * @param quad: The quad.
 * @return: The quad's node.
 */
yf_node_t *yf_quad_getnode(yf_quad_t *quad);

/**
 * Gets the mesh of a quad.
 *
 * @param quad: The quad.
 * @return: The mesh used by the quad.
 */
yf_mesh_t *yf_quad_getmesh(yf_quad_t *quad);

/**
 * Gets the texture of a quad.
 *
 * @param quad: The quad.
 * @return: The texture used by the quad, or 'NULL' if none is set.
 */
yf_texture_t *yf_quad_gettex(yf_quad_t *quad);

/**
 * Sets the texture for a quad.
 *
 * @param quad: The quad.
 * @param tex: The texture to set. Can be 'NULL'.
 */
void yf_quad_settex(yf_quad_t *quad, yf_texture_t *tex);

/**
 * Gets the rectangle of a quad.
 *
 * This rectangle defines a range of the currently set texture that contains
 * the quad. The default value is a rect that encloses the whole texture.
 *
 * @param quad: The quad.
 * @return: The quad's rectangle.
 */
const yf_rect_t *yf_quad_getrect(yf_quad_t *quad);

/**
 * Sets the rectangle for a quad.
 *
 * Setting a new texture overrides the value set by a call to this function.
 *
 * NOTE: One must ensure that 'rect' specifies a subrange of the current
 * texture.
 *
 * @param quad: The quad.
 * @param rect: The rectangle to set.
 */
void yf_quad_setrect(yf_quad_t *quad, const yf_rect_t *rect);

/**
 * Gets the color of quad.
 *
 * @param quad: The quad.
 * @param corner: The 'YF_CORNER' value indicating the corner from which to
 *  retrieve the color.
 * @return: The color currently set for 'corner'.
 */
yf_color_t yf_quad_getcolor(yf_quad_t *quad, int corner);

/**
 * Sets the color for a quad.
 *
 * @param quad: The quad.
 * @param corner_mask: A mask of 'YF_CORNER' values indicating the corners
 *  to be updated with 'color'.
 * @param color: The color to set.
 */
void yf_quad_setcolor(yf_quad_t *quad, unsigned corner_mask, yf_color_t color);

/**
 * Deinitializes a quad.
 *
 * @param quad: The quad to deinitialize. Can be 'NULL'.
 */
void yf_quad_deinit(yf_quad_t *quad);

YF_DECLS_END

#endif /* YF_YF_QUAD_H */
