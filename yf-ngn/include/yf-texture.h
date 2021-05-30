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

YF_DECLS_BEGIN

/**
 * Opaque type defining a texture resource.
 */
typedef struct YF_texture_o *YF_texture;

/**
 * Texture file types.
 */
#define YF_FILETYPE_UNKNOWN  0
#define YF_FILETYPE_INTERNAL 1
#define YF_FILETYPE_PNG      48
#define YF_FILETYPE_BMP      49

/**
 * Initializes a new texture.
 *
 * @param filetype: The 'YF_FILETYPE' value indicating the format of the
 *  texture file.
 * @param pathname: The pathname of the texture file.
 * @return: On success, returns a new texture. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_texture yf_texture_init(int filetype, const char *pathname);

/**
 * Gets the dimensions of a texture.
 *
 * @param tex: The texture.
 * @return: The texture dimensions, in pixels.
 */
YF_dim2 yf_texture_getdim(YF_texture tex);

/**
 * Deinitializes a texture.
 *
 * @param tex: The texture to deinitialize. Can be 'NULL'.
 */
void yf_texture_deinit(YF_texture tex);

YF_DECLS_END

#endif /* YF_YF_TEXTURE_H */
