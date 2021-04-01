/*
 * YF
 * yf-model.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_MODEL_H
#define YF_YF_MODEL_H

#include <yf/com/yf-defs.h>

#include "yf-node.h"
#include "yf-mesh.h"
#include "yf-texture.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a drawable model.
 */
typedef struct YF_model_o *YF_model;

/**
 * Initializes a new model.
 *
 * @return: On success, returns a new model. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_model yf_model_init(void);

/**
 * Gets the node of a model.
 *
 * @param mdl: The model.
 * @return: The model's node.
 */
YF_node yf_model_getnode(YF_model mdl);

/**
 * Gets the mesh of a model.
 *
 * @param mdl: The model.
 * @return: The mesh used by the model, or 'NULL' if none is set.
 */
YF_mesh yf_model_getmesh(YF_model mdl);

/**
 * Sets the mesh for a model.
 *
 * @param mdl: The model.
 * @param mesh: The mesh to set. Can be 'NULL'.
 */
void yf_model_setmesh(YF_model mdl, YF_mesh mesh);

/**
 * Gets the texture of a model.
 *
 * @param mdl: The model.
 * @return: The texture used by the model, or 'NULL' if none is set.
 */
YF_texture yf_model_gettex(YF_model mdl);

/**
 * Sets the texture for a model.
 *
 * @param mdl: The model.
 * @param tex: The texture to set. Can be 'NULL'.
 */
void yf_model_settex(YF_model mdl, YF_texture tex);

/**
 * Deinitializes a model.
 *
 * @param mdl: The model to deinitialize. Can be 'NULL'.
 */
void yf_model_deinit(YF_model mdl);

YF_DECLS_END

#endif /* YF_YF_MODEL_H */
