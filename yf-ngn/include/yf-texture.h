/*
 * YF
 * yf-texture.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_TEXTURE_H
#define YF_YF_TEXTURE_H

#include "yf/com/yf-defs.h"
#include "yf/com/yf-types.h"
#include "yf/core/yf-sampler.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a texture.
 */
typedef struct YF_texture_o *YF_texture;

/**
 * Texture coordinate sets.
 */
#define YF_UVSET_0 0
#define YF_UVSET_1 1

/**
 * Initializes a new texture.
 *
 * @param pathname: The pathname of the texture file.
 * @param index: The index of the texture to load.
 * @param splr: The sampler to use with the texture. Can be 'NULL'.
 * @return: On success, returns a new texture. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_texture yf_texture_init(const char *pathname, size_t index,
                           const YF_sampler *splr);

/**
 * Gets the dimensions of a texture.
 *
 * @param tex: The texture.
 * @return: The texture dimensions, in pixels.
 */
YF_dim2 yf_texture_getdim(YF_texture tex);

/**
 * Gets the sampler of a texture.
 *
 * @param tex: The texture.
 * @return: The texture sampler.
 */
YF_sampler *yf_texture_getsplr(YF_texture tex);

/**
 * Deinitializes a texture.
 *
 * @param tex: The texture to deinitialize. Can be 'NULL'.
 */
void yf_texture_deinit(YF_texture tex);

YF_DECLS_END

#endif /* YF_YF_TEXTURE_H */
