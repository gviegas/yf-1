/*
 * YF
 * yf-model.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_MODEL_H
#define YF_YF_MODEL_H

#include "yf-common.h"
#include "yf-node.h"
#include "yf-matrix.h"
#include "yf-mesh.h"
#include "yf-texture.h"

YF_DECLS_BEGIN

/* Opaque type defining a drawable model. */
typedef struct YF_model_o *YF_model;

/* Initializes a new model. */
YF_model yf_model_init(void);

/* Gets the node of a model. */
YF_node yf_model_getnode(YF_model mdl);

/* Gets the transformation matrix of a model. */
YF_mat4 *yf_model_getxform(YF_model mdl);

/* Gets the mesh of a model. */
YF_mesh yf_model_getmesh(YF_model mdl);

/* Sets the mesh for a model. */
void yf_model_setmesh(YF_model mdl, YF_mesh mesh);

/* Gets the texture of a model. */
YF_texture yf_model_gettex(YF_model mdl);

/* Sets the texture for a model. */
void yf_model_settex(YF_model mdl, YF_texture tex);

/* Deinitializes a model. */
void yf_model_deinit(YF_model mdl);

YF_DECLS_END

#endif /* YF_YF_MODEL_H */
