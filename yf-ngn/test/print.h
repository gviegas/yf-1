/*
 * YF
 * print.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_PRINT_H
#define YF_PRINT_H

#include "node.h"
#include "mesh.h"
#include "texture.h"
#include "yf-animation.h"

void yf_print_nodeobj(YF_node node);
void yf_print_mesh(YF_mesh mesh);
void yf_print_tex(YF_texture tex);
void yf_print_anim(YF_animation anim);

#endif /* YF_PRINT_H */
