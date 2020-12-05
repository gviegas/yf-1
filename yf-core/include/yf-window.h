/*
 * YF
 * yf-window.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_WINDOW_H
#define YF_YF_WINDOW_H

#include "yf-common.h"
#include "yf-context.h"
#include "yf-pass.h"

YF_DECLS_BEGIN

/* Opaque type defining a window. */
typedef struct YF_window_o *YF_window;

/* Initializes a new window. */
YF_window yf_window_init(YF_context ctx, YF_dim2 dim, const char *title);

/* Gets the current dimensions of a window. */
YF_dim2 yf_window_getdim(YF_window win);

/* Gets the color descriptor of a window. */
const YF_colordsc *yf_window_getdsc(YF_window win);

/* Gets the list of all attachments of a n-buffered window. */
const YF_attach *yf_window_getatts(YF_window win, unsigned *n);

/* Gets the index of the next writable attachment. */
int yf_window_next(YF_window win);

/* Presents the contents of the next attachment. */
int yf_window_present(YF_window win);

/* Resizes a window. */
int yf_window_resize(YF_window win, YF_dim2 dim);

/* Closes a window. */
int yf_window_close(YF_window win);

/* Recreates a window. */
int yf_window_recreate(YF_window win);

/* Deinitializes a window. */
void yf_window_deinit(YF_window win);

YF_DECLS_END

#endif /* YF_YF_WINDOW_H */
