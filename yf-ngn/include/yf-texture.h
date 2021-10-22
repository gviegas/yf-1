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

#include "yf-collection.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a texture.
 */
typedef struct YF_texture_o *YF_texture;

/**
 * Loads a new texture from file.
 *
 * @param pathname: The pathname of the texture file.
 * @param index: The index of the texture to load.
 * @param coll: The collection for the texture.
 * @return: On success, returns a new texture. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_texture yf_texture_load(const char *pathname, size_t index,
                           YF_collection coll);

/**
 * Texture coordinate sets.
 */
#define YF_UVSET_0 0
#define YF_UVSET_1 1

/**
 * Type defining texture data.
 */
typedef struct {
    void *data;
    int pixfmt;
    YF_dim2 dim;
    YF_sampler splr;
    int uvset;
} YF_texdt;

/**
 * Type defining a texture with custom sampler and coordinate set.
 */
typedef struct {
    YF_texture tex;
    YF_sampler splr;
    int uvset;
} YF_texref;

/**
 * Gets the default reference of a texture.
 *
 * @param tex: The texture.
 * @param ref: The destination for the reference.
 * @return: 'ref'.
 */
YF_texref *yf_texture_getref(YF_texture tex, YF_texref *ref);

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
 * Gets the 'UVSET' value of a texture.
 *
 * @param tex: The texture.
 * @return: The texture 'UVSET'.
 */
int yf_texture_getuv(YF_texture tex);

/**
 * Sets the 'UVSET' value for a texture.
 *
 * @param tex: The texture.
 * @param uvset: The 'UVSET' value to set.
 */
void yf_texture_setuv(YF_texture tex, int uvset);

/**
 * Deinitializes a texture.
 *
 * @param tex: The texture to deinitialize. Can be 'NULL'.
 */
void yf_texture_deinit(YF_texture tex);

YF_DECLS_END

#endif /* YF_YF_TEXTURE_H */
