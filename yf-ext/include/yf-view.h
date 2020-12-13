/*
 * YF
 * yf-view.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_VIEW_H
#define YF_YF_VIEW_H

#include <yf/wsys/yf-window.h>

#include "yf-common.h"
#include "yf-scene.h"

YF_DECLS_BEGIN

/* Opaque type defining the view. */
typedef struct YF_view_o *YF_view;

/* Initializes a new view. */
YF_view yf_view_init(YF_window win);

/* Sets a view's scene. */
void yf_view_setscene(YF_view view, YF_scene scn);

/* Renders the current scene of a view. */
int yf_view_render(YF_view view);

/* Starts a view's rendering loop. */
int yf_view_start(YF_view view, unsigned fps,
    void (*update)(double elapsed_time));

/* Stops a view's rendering loop. */
void yf_view_stop(YF_view view);

/* Deinitializes a view. */
void yf_view_deinit(YF_view view);

YF_DECLS_END

#endif /* YF_YF_VIEW_H */
