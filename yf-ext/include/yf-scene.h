/*
 * YF
 * yf-scene.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_SCENE_H
#define YF_YF_SCENE_H

#include <yf/com/yf-defs.h>

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
typedef struct YF_scene_o *YF_scene;

/**
 * Initializes a new scene.
 *
 * @return: On success, returns a new scene. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_scene yf_scene_init(void);

/**
 * Gets the root node of a scene.
 *
 * Only nodes that descend from a scene's root node will be rendered when
 * rendering the scene.
 * The location of a node in the scene graph has no practical ordering
 * implications.
 *
 * @param scn: The scene.
 * @return: The scene's node.
 */
YF_node yf_scene_getnode(YF_scene scn);

/**
 * Gets the camera of a scene.
 *
 * @param scn: The scene.
 * @return: The scene's camera.
 */
YF_camera yf_scene_getcam(YF_scene scn);

/**
 * Gets the color used when clearing a scene for rendering.
 *
 * @param scn: The scene.
 * @return: The color value.
 */
YF_color yf_scene_getcolor(YF_scene scn);

/**
 * Sets the color to use when clearing a scene for rendering.
 *
 * @param scn: The scene.
 * @param color: The color value to set.
 */
void yf_scene_setcolor(YF_scene scn, YF_color color);

/**
 * Deinitializes a scene.
 *
 * @param scn: The scene to deinitialize. Can be 'NULL'.
 */
void yf_scene_deinit(YF_scene scn);

YF_DECLS_END

#endif /* YF_YF_SCENE_H */
