/*
 * YF
 * yf-scene.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_SCENE_H
#define YF_YF_SCENE_H

#include "yf/com/yf-defs.h"
#include "yf/com/yf-types.h"

#include "yf-node.h"
#include "yf-camera.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a scene graph.
 *
 * The scene graph is a collection of nodes. One can insert any number of
 * nodes in a scene - the underlying objects represented by these nodes
 * will be rendered when rendering the scene itself.
 */
typedef struct yf_scene yf_scene_t;

/**
 * Initializes a new scene.
 *
 * @return: On success, returns a new scene. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_scene_t *yf_scene_init(void);

/**
 * Gets the root node of a scene.
 *
 * Only nodes that descend from a scene's root node will be rendered when
 * rendering the scene.
 *
 * @param scn: The scene.
 * @return: The scene's node.
 */
yf_node_t *yf_scene_getnode(yf_scene_t *scn);

/**
 * Gets the camera of a scene.
 *
 * @param scn: The scene.
 * @return: The scene's camera.
 */
yf_camera_t *yf_scene_getcam(yf_scene_t *scn);

/**
 * Gets the color used when clearing a scene for rendering.
 *
 * @param scn: The scene.
 * @return: The color value.
 */
yf_color_t yf_scene_getcolor(yf_scene_t *scn);

/**
 * Sets the color to use when clearing a scene for rendering.
 *
 * @param scn: The scene.
 * @param color: The color value to set.
 */
void yf_scene_setcolor(yf_scene_t *scn, yf_color_t color);

/**
 * Deinitializes a scene.
 *
 * NOTE: One must ensure that the scene is not set for any views before
 * calling this function.
 *
 * @param scn: The scene to deinitialize. Can be 'NULL'.
 */
void yf_scene_deinit(yf_scene_t *scn);

YF_DECLS_END

#endif /* YF_YF_SCENE_H */
