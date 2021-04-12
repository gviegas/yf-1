/*
 * YF
 * yf-wsi.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_WSI_H
#define YF_YF_WSI_H

#include "yf/com/yf-defs.h"
#include "yf/wsys/yf-window.h"

#include "yf-context.h"
#include "yf-image.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a presentable surface.
 */
typedef struct YF_wsi_o *YF_wsi;

/**
 * Initializes a new wsi.
 *
 * @param ctx: The context.
 * @param win: The window into which present results.
 * @return: On success, returns 0. Otherwise, 'NULL' is returned and the
 * global error is set to indicate the cause.
 */
YF_wsi yf_wsi_init(YF_context ctx, YF_window win);

/**
 * Gets the list of all images in the wsi's swapchain.
 *
 * @param wsi: The wsi.
 * @param n: The destination for the number of images in the returned array.
 * @return: On success, returns an array containing the wsi images. Otherwise,
 *  'NULL' is returned and the global error is set to indicate the cause.
 */
const YF_image *yf_wsi_getimages(YF_wsi wsi, unsigned *n);

/**
 * Gets the maximum number of images that can be acquired.
 *
 * When this limit is greater than one, multiple images can be held at once,
 * prior to presentation. Otherwise, one needs to submit the acquired image
 * for presentation before acquiring another one.
 *
 * @param wsi: The wsi to query.
 * @return: The acquisition limit. For a valid wsi object, this value will be
 *  at least one.
 */
unsigned yf_wsi_getlimit(YF_wsi wsi);

/**
 * Queries the index of the next writable image.
 *
 * The acquired image corresponds to the one retrieved from
 * 'yf_wsi_getimages()' using this function's return value as an index.
 *
 * @param wsi: The wsi.
 * @param nonblocking: Whether or not the call will block waiting for an image.
 * @return: On success, returns the index of the image that can be written.
 *  Otherwise, a negative value is returned and the global error is set to
 *  indicate the cause.
 */
int yf_wsi_next(YF_wsi wsi, int nonblocking);

/**
 * Presents a previously acquired image.
 *
 * @param wsi: The wsi.
 * @param index: The index of the image to present.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_wsi_present(YF_wsi wsi, unsigned index);

/**
 * Deinitializes a wsi.
 *
 * @param wsi: The wsi to deinitialize. Can be 'NULL'.
 */
void yf_wsi_deinit(YF_wsi wsi);

YF_DECLS_END

#endif /* YF_YF_WSI_H */
