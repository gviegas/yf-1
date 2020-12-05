/*
 * YF
 * yf-texture.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_TEXTURE_H
#define YF_YF_TEXTURE_H

#include "yf-common.h"

YF_DECLS_BEGIN

/* Opaque type defining the texture data. */
typedef struct YF_texture_o *YF_texture;

/* Initializes a new texture. */
YF_texture yf_texture_init(int filetype, const char *pathname);

/* Deinitializes a texture. */
void yf_texture_deinit(YF_texture tex);

YF_DECLS_END

#endif /* YF_YF_TEXTURE_H */
