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

/* Opaque type defining a scene graph. */
typedef struct YF_scene_o *YF_scene;

/* Initializes a new scene. */
YF_scene yf_scene_init(void);

/* Gets the root node of a scene. */
YF_node yf_scene_getnode(YF_scene scn);

/* Gets the camera of a scene. */
YF_camera yf_scene_getcam(YF_scene scn);

/* Gets the color used when clearing a scene for rendering. */
YF_color yf_scene_getcolor(YF_scene scn);

/* Sets the color to use when clearing a scene for rendering. */
void yf_scene_setcolor(YF_scene scn, YF_color color);

/* Deinitializes a scene. */
void yf_scene_deinit(YF_scene scn);

YF_DECLS_END

#endif /* YF_YF_SCENE_H */
