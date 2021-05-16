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
 * Starts a view's rendering loop.
 *
 * This function implicitly calls 'yf_view_render()' on every loop iteration,
 * thus one should not attempt to render a scene on the provided callback.
 *
 * Setting a different scene from the 'update' callback is allowed. The view
 * will start rendering the new scene as soon as the callback returns.
 *
 * @param view: The view.
 * @param fps: The preferred FPS.
 * @param update: The function to call before rendering takes place.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_view_start(YF_view view, unsigned fps,
                  void (*update)(double elapsed_time));

/**
 * Stops a view's rendering loop.
 *
 * @param view: The view.
 */
void yf_view_stop(YF_view view);

/**
 * Deinitializes a view.
 *
 * @param view: The view to deinitialize. Can be 'NULL'.
 */
void yf_view_deinit(YF_view view);

YF_DECLS_END

#endif /* YF_YF_VIEW_H */
