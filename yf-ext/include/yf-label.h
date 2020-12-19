/*
 * YF
 * yf-label.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_LABEL_H
#define YF_YF_LABEL_H

#include <yf/com/yf-defs.h>

#include "yf-node.h"
#include "yf-matrix.h"
#include "yf-mesh.h"
#include "yf-texture.h"

YF_DECLS_BEGIN

/* TODO */

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
 * Deinitializes a label.
 *
 * @param labl: The label to deinitialize. Can be 'NULL'.
 */
void yf_label_deinit(YF_label labl);

YF_DECLS_END

#endif /* YF_YF_LABEL_H */
