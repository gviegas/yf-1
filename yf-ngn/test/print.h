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
#include "yf-kfanim.h"
#include "yf-collec.h"

void yf_print_nodeobj(yf_node_t *);
void yf_print_mesh(yf_mesh_t *);
void yf_print_tex(yf_texture_t *);
void yf_print_skin(yf_skin_t *);
void yf_print_matl(yf_material_t *);
void yf_print_cam(yf_camera_t *);
void yf_print_anim(yf_kfanim_t *);
void yf_print_coll(yf_collec_t *);

#endif /* YF_PRINT_H */
