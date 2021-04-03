/*
 * YF
 * scene.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_SCENE_H
#define YF_SCENE_H

#include "yf/core/yf-pass.h"

#include "yf-scene.h"

/* Renders a scene. */
int yf_scene_render(YF_scene scn, YF_pass pass, YF_target tgt, YF_dim2 dim);

#endif /* YF_SCENE_H */
