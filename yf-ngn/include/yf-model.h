/*
 * YF
 * yf-model.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_MODEL_H
#define YF_YF_MODEL_H

#include "yf/com/yf-defs.h"

#include "yf-node.h"
#include "yf-mesh.h"
#include "yf-skin.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a drawable model.
 */
typedef struct yf_model yf_model_t;

/**
 * Initializes a new model.
 *
 * @return: On success, returns a new model. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_model_t *yf_model_init(void);

/**
 * Gets the node of a model.
 *
 * @param mdl: The model.
 * @return: The model's node.
 */
yf_node_t *yf_model_getnode(yf_model_t *mdl);

/**
 * Gets the mesh of a model.
 *
 * @param mdl: The model.
 * @return: The mesh used by the model, or 'NULL' if none is set.
 */
yf_mesh_t *yf_model_getmesh(yf_model_t *mdl);

/**
 * Sets the mesh for a model.
 *
 * @param mdl: The model.
 * @param mesh: The mesh to set. Can be 'NULL'.
 */
void yf_model_setmesh(yf_model_t *mdl, yf_mesh_t *mesh);

/**
 * Gets the skin of a model.
 *
 * @param mdl: The model.
 * @param skel: The destination for the skin's skeleton.
 * @return: The skin used by the model, or 'NULL' if none is set.
 */
yf_skin_t *yf_model_getskin(yf_model_t *mdl, yf_skeleton_t **skel);

/**
 * Sets the skin for a model.
 *
 * @param mdl: The model.
 * @param skin: The skin to set. Can be 'NULL'.
 * @param skel: The skeleton instance to set. Can be 'NULL'.
 */
void yf_model_setskin(yf_model_t *mdl, yf_skin_t *skin, yf_skeleton_t *skel);

/**
 * Deinitializes a model.
 *
 * @param mdl: The model to deinitialize. Can be 'NULL'.
 */
void yf_model_deinit(yf_model_t *mdl);

YF_DECLS_END

#endif /* YF_YF_MODEL_H */
