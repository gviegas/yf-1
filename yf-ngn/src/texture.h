/*
 * YF
 * texture.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_TEXTURE_H
#define YF_TEXTURE_H

#include "yf/core/yf-cmdbuf.h"

#include "yf-texture.h"

/* Type defining the texture data. */
typedef struct {
    void *data;
    int pixfmt;
    YF_dim2 dim;
    YF_sampler splr;
} YF_texdt;

/* Initializes a new texture object from texture data directly. */
YF_texture yf_texture_initdt(const YF_texdt *data);

/* Replaces the contents of a texture object's image. */
int yf_texture_setdata(YF_texture tex, YF_off2 off, YF_dim2 dim,
                       const void *data);

/* Copies image data from a texture to a dtable resource. */
int yf_texture_copyres(YF_texture tex, YF_dtable dtb, unsigned alloc_i,
                       unsigned binding, unsigned element);

#endif /* YF_TEXTURE_H */
