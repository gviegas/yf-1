/*
 * YF
 * scene.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_SCENE_H
#define YF_SCENE_H

#include "yf/core/yf-pass.h"

#include "yf-scene.h"

/* Renders a scene. */
int yf_scene_render(yf_scene_t *scn, yf_pass_t *pass, yf_target_t *tgt,
                    yf_dim2_t dim);

#endif /* YF_SCENE_H */
