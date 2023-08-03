/*
 * YF
 * texture.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_TEXTURE_H
#define YF_TEXTURE_H

#include "yf/core/yf-cmdbuf.h"

#include "yf-texture.h"

/* Replaces the contents of a texture object's image. */
int yf_texture_setdata(yf_texture_t *tex, yf_off2_t off, yf_dim2_t dim,
                       const void *data);

/* Copies image data from a texture to a dtable resource. */
int yf_texture_copyres(yf_texture_t *tex, yf_dtable_t *dtb, unsigned alloc_i,
                       unsigned binding, unsigned element);

/* Copies image data from a texture reference to a dtable resource. */
int yf_texture_copyres2(const yf_texref_t *ref, yf_dtable_t *dtb,
                        unsigned alloc_i, unsigned binding, unsigned element);

#endif /* YF_TEXTURE_H */
