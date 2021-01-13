/*
 * YF
 * yf-label.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_LABEL_H
#define YF_YF_LABEL_H

#include <stddef.h>

#include <yf/com/yf-defs.h>

#include "yf-node.h"
#include "yf-matrix.h"
#include "yf-mesh.h"
#include "yf-texture.h"
#include "yf-font.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a drawable text label.
 */
typedef struct YF_label_o *YF_label;

/**
 * Initializes a new label.
 *
 * @return: On success, returns a new label. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_label yf_label_init(void);

/**
 * Gets the node of a label.
 *
 * @param labl: The label.
 * @return: The label's node.
 */
YF_node yf_label_getnode(YF_label labl);

/**
 * Gets the transformation matrix of a label.
 *
 * @param labl: The label.
 * @return: The label's transformation matrix.
 */
YF_mat4 *yf_label_getxform(YF_label labl);

/**
 * Gets the mesh of a label.
 *
 * @param labl: The label.
 * @return: The mesh used by the label.
 */
YF_mesh yf_label_getmesh(YF_label labl);

/**
 * Gets the texture of a label.
 *
 * @param labl: The label.
 * @return: The texture used by the label.
 */
YF_texture yf_label_gettex(YF_label labl);

/**
 * Gets the font of a label.
 *
 * @param labl: The label.
 * @return: The font used by the label, or 'NULL' if none is set.
 */
YF_font yf_label_getfont(YF_label labl);

/**
 * Sets the font for a label.
 *
 * @param labl: The label.
 * @param font: The font to set. Can be 'NULL'.
 */
void yf_label_setfont(YF_label labl, YF_font font);

/**
 * Gets the string of a label.
 *
 * @param labl: The label.
 * @param dst: The destination for the string.
 * @param n: The number of wide characters that the destination buffer can
 *  contain. Must be at least one.
 * @return: If the length of the string exceeds 'n', returns 'NULL'.
 *  Otherwise, returns 'dst'.
 */
wchar_t *yf_label_getstr(YF_label labl, wchar_t *dst, size_t n);

/**
 * Sets the string for a label.
 *
 * @param labl: The label.
 * @param str: The string to set. Can be 'NULL'.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_label_setstr(YF_label labl, wchar_t *str);

/**
 * Deinitializes a label.
 *
 * @param labl: The label to deinitialize. Can be 'NULL'.
 */
void yf_label_deinit(YF_label labl);

YF_DECLS_END

#endif /* YF_YF_LABEL_H */
