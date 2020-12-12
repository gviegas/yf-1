/*
 * YF
 * yf-wsi.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_WSI_H
#define YF_YF_WSI_H

#include <yf/wsys/yf-window.h>

#include "yf-common.h"
#include "yf-context.h"
#include "yf-image.h"

YF_DECLS_BEGIN

/* Opaque type defining a presentable surface. */
typedef struct YF_wsi_o *YF_wsi;

/* Initializes a new wsi. */
YF_wsi yf_wsi_init(YF_context ctx, YF_window win);

/* Gets the list of all images in the swapchain. */
const YF_image *yf_wsi_getimages(YF_wsi wsi, unsigned *n);

/* Gets the maximum number of images that can be acquired. */
unsigned yf_wsi_getlimit(YF_wsi wsi);

/* Gets the index of the next writable image. */
int yf_wsi_getindex(YF_wsi wsi, int nonblocking);

/* Presents a previously acquired image. */
int yf_wsi_present(YF_wsi wsi, unsigned index);

/* Deinitializes a wsi. */
void yf_wsi_deinit(YF_wsi wsi);

YF_DECLS_END

#endif /* YF_YF_WSI_H */
