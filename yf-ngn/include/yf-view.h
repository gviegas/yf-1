/*
 * YF
 * yf-view.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_VIEW_H
#define YF_YF_VIEW_H

#include "yf/com/yf-defs.h"
#include "yf/wsys/yf-window.h"

#include "yf-scene.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining the view.
 *
 * A view is responsible for issuing scene rendering requests and for
 * presenting the results on its associated window.
 */
typedef struct YF_view_o *YF_view;

/**
 * Initializes a new view.
 *
 * If a view already exists, this function fails and sets the global error
 * to 'YF_ERR_EXIST'.
 *
 * @param win: The window to associate the view with.
 * @return: On success, returns a new view. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_view yf_view_init(YF_window win);

/**
 * Starts a view's rendering loop.
 *
 * @param view: The view.
 * @param scn: The scene to render.
 * @param fps: The preferred FPS.
 * @param update: The function to call before rendering takes place.
 * @param arg: The generic argument to pass on 'update' calls. Can be 'NULL'.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_view_loop(YF_view view, YF_scene scn, unsigned fps,
                 int (*update)(double elapsed_time, void *arg), void *arg);

/**
 * Gets a view's scene.
 *
 * @param view: The view.
 * @return: The scene currently set for 'view', or 'NULL' if none is set.
 */
YF_scene yf_view_getscene(YF_view view);

/**
 * Sets a view's scene.
 *
 * @param view: The view.
 * @param scn: The scene to set. Can be 'NULL'.
 */
void yf_view_setscene(YF_view view, YF_scene scn);

/**
 * Renders the current scene of a view.
 *
 * @param view: The view.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_view_render(YF_view view);

/**
 * Deinitializes a view.
 *
 * @param view: The view to deinitialize. Can be 'NULL'.
 */
void yf_view_deinit(YF_view view);

YF_DECLS_END

#endif /* YF_YF_VIEW_H */
