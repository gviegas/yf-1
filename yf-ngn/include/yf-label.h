/*
 * YF
 * yf-label.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_LABEL_H
#define YF_YF_LABEL_H

#include <stddef.h>

#include "yf/com/yf-defs.h"

#include "yf-node.h"
#include "yf-mesh.h"
#include "yf-texture.h"
#include "yf-font.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a drawable text label.
 */
typedef struct yf_label yf_label_t;

/**
 * Initializes a new label.
 *
 * @return: On success, returns a new label. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_label_t *yf_label_init(void);

/**
 * Gets the node of a label.
 *
 * @param labl: The label.
 * @return: The label's node.
 */
yf_node_t *yf_label_getnode(yf_label_t *labl);

/**
 * Gets the mesh of a label.
 *
 * @param labl: The label.
 * @return: The mesh used by the label.
 */
yf_mesh_t *yf_label_getmesh(yf_label_t *labl);

/**
 * Gets the texture of a label.
 *
 * @param labl: The label.
 * @return: The texture used by the label.
 */
yf_texture_t *yf_label_gettex(yf_label_t *labl);

/**
 * Gets the font of a label.
 *
 * @param labl: The label.
 * @return: The font used by the label, or 'NULL' if none is set.
 */
yf_font_t *yf_label_getfont(yf_label_t *labl);

/**
 * Sets the font for a label.
 *
 * @param labl: The label.
 * @param font: The font to set. Can be 'NULL'.
 */
void yf_label_setfont(yf_label_t *labl, yf_font_t *font);

/**
 * Gets the string of a label.
 *
 * @param labl: The label.
 * @param dst: The destination for the string. Can be 'NULL'.
 * @param n: The number of wide characters that 'dst' can contain. When 'dst'
 *  is not 'NULL', '*n' must be greater than zero. This location is updated
 *  to contain the string size, including the terminating null wide character.
 * @return: If 'dst' is not 'NULL' and the length of the string (including the
 *  terminating null wide character) is less than or equal '*n', returns 'dst'.
 *  Otherwise, 'NULL' is returned and no copy is performed.
 */
wchar_t *yf_label_getstr(yf_label_t *labl, wchar_t *dst, size_t *n);

/**
 * Sets the string for a label.
 *
 * @param labl: The label.
 * @param str: The string to set. Can be 'NULL'.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_label_setstr(yf_label_t *labl, const wchar_t *str);

/**
 * Gets the font size of a label.
 *
 * @param labl: The label.
 * @return: The size used for the label's font, in points.
 */
unsigned short yf_label_getpt(yf_label_t *labl);

/**
 * Sets the font size for a label.
 *
 * @param labl: The label.
 * @param pt: The size to use for the label's font, in points.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_label_setpt(yf_label_t *labl, unsigned short pt);

/**
 * Gets the color of a label.
 *
 * @param labl: The label.
 * @param corner: The 'YF_CORNER' value indicating the corner from which to
 *  retrieve the color.
 * @return: The color currently set for 'corner'.
 */
yf_color_t yf_label_getcolor(yf_label_t *labl, int corner);

/**
 * Sets the color for a label.
 *
 * @param labl: The label.
 * @param corner_mask: A mask of 'YF_CORNER' values indicating the corners
 *  to be updated with 'color'.
 * @param color: The color to set.
 */
void yf_label_setcolor(yf_label_t *labl, unsigned corner_mask,
                       yf_color_t color);

/**
 * Gets the dimensions of a label.
 *
 * @param labl: The label.
 * @return: The dimensions of the label's rasterized output, in pixels.
 */
yf_dim2_t yf_label_getdim(yf_label_t *labl);

/**
 * Deinitializes a label.
 *
 * @param labl: The label to deinitialize. Can be 'NULL'.
 */
void yf_label_deinit(yf_label_t *labl);

YF_DECLS_END

#endif /* YF_YF_LABEL_H */
