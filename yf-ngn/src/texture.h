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

/* Replaces the contents of a texture object's image. */
int yf_texture_setdata(YF_texture tex, YF_off2 off, YF_dim2 dim,
                       const void *data);

/* Copies image data from a texture to a dtable resource. */
int yf_texture_copyres(YF_texture tex, YF_dtable dtb, unsigned alloc_i,
                       unsigned binding, unsigned element);

/* Copies image data from a texture reference to a dtable resource. */
int yf_texture_copyres2(const YF_texref *ref, YF_dtable dtb, unsigned alloc_i,
                        unsigned binding, unsigned element);

#endif /* YF_TEXTURE_H */
