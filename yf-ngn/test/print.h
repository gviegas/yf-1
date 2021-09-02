/*
 * YF
 * print.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_PRINT_H
#define YF_PRINT_H

#include "yf-node.h"
#include "yf-mesh.h"
#include "yf-texture.h"
#include "yf-skin.h"
#include "yf-material.h"
#include "yf-camera.h"
#include "yf-animation.h"
#include "yf-collection.h"

void yf_print_nodeobj(YF_node);
void yf_print_mesh(YF_mesh);
void yf_print_tex(YF_texture);
void yf_print_skin(YF_skin);
void yf_print_matl(YF_material);
void yf_print_cam(YF_camera);
void yf_print_anim(YF_animation);
void yf_print_coll(YF_collection);

#endif /* YF_PRINT_H */
