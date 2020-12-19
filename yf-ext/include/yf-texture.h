/*
 * YF
 * yf-texture.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_TEXTURE_H
#define YF_YF_TEXTURE_H

#include <yf/com/yf-defs.h>

YF_DECLS_BEGIN

/**
 * Opaque type defining the texture data.
 */
typedef struct YF_texture_o *YF_texture;

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
 * Deinitializes a texture.
 *
 * @param tex: The texture to deinitialize. Can be 'NULL'.
 */
void yf_texture_deinit(YF_texture tex);

YF_DECLS_END

#endif /* YF_YF_TEXTURE_H */
