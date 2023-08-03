/*
 * YF
 * yf-texture.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_TEXTURE_H
#define YF_YF_TEXTURE_H

#include <stddef.h>

#include "yf/com/yf-defs.h"
#include "yf/com/yf-types.h"
#include "yf/core/yf-sampler.h"

#include "yf-collec.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a texture.
 */
typedef struct yf_texture yf_texture_t;

/**
 * Loads a new texture from file.
 *
 * @param pathname: The pathname of the texture file.
 * @param index: The index of the texture to load.
 * @param coll: The collection for the texture.
 * @return: On success, returns a new texture. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_texture_t *yf_texture_load(const char *pathname, size_t index,
                              yf_collec_t *coll);

/**
 * Texture coordinate sets.
 */
#define YF_UVSET_0 0
#define YF_UVSET_1 1

/**
 * Type defining texture data.
 */
typedef struct yf_texdt {
    void *data;
    int pixfmt;
    yf_dim2_t dim;
    yf_sampler_t splr;
    int uvset;
} yf_texdt_t;

/**
 * Initializes a new texture.
 *
 * @param data: The data from which to initialize the texture.
 * @return: On success, returns a new texture. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_texture_t *yf_texture_init(const yf_texdt_t *data);

/**
 * Type defining a texture with custom sampler and coordinate set.
 */
typedef struct {
    yf_texture_t *tex;
    yf_sampler_t splr;
    int uvset;
} yf_texref_t;

/**
 * Gets the default reference of a texture.
 *
 * @param tex: The texture.
 * @param ref: The destination for the reference.
 * @return: 'ref'.
 */
yf_texref_t *yf_texture_getref(yf_texture_t *tex, yf_texref_t *ref);

/**
 * Gets the dimensions of a texture.
 *
 * @param tex: The texture.
 * @return: The texture dimensions, in pixels.
 */
yf_dim2_t yf_texture_getdim(yf_texture_t *tex);

/**
 * Gets the sampler of a texture.
 *
 * @param tex: The texture.
 * @return: The texture sampler.
 */
yf_sampler_t *yf_texture_getsplr(yf_texture_t *tex);

/**
 * Gets the 'UVSET' value of a texture.
 *
 * @param tex: The texture.
 * @return: The texture 'UVSET'.
 */
int yf_texture_getuv(yf_texture_t *tex);

/**
 * Sets the 'UVSET' value for a texture.
 *
 * @param tex: The texture.
 * @param uvset: The 'UVSET' value to set.
 */
void yf_texture_setuv(yf_texture_t *tex, int uvset);

/**
 * Deinitializes a texture.
 *
 * @param tex: The texture to deinitialize. Can be 'NULL'.
 */
void yf_texture_deinit(yf_texture_t *tex);

YF_DECLS_END

#endif /* YF_YF_TEXTURE_H */
