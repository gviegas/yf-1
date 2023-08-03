/*
 * YF
 * yf-view.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
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
typedef struct yf_view yf_view_t;

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
yf_view_t *yf_view_init(yf_window_t *win);

/**
 * Starts a view's rendering loop.
 *
 * This function executes the provided callback once per frame, before calling
 * 'view_render()' to render the scene. The loop ends when 'update' returns a
 * non-zero value, or if an error occurs during rendering.
 *
 * @param view: The view.
 * @param scn: The scene to render.
 * @param fps: The preferred FPS.
 * @param update: The function to call before rendering takes place.
 * @param arg: The generic argument to pass on 'update' calls. Can be 'NULL'.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
/* TODO: Replace 'fps' arg with 'vsync' and fix the pacing. */
int yf_view_loop(yf_view_t *view, yf_scene_t *scn, unsigned fps,
                 int (*update)(double elapsed_time, void *arg), void *arg);

/**
 * Swaps the scene in a view's loop.
 *
 * NOTE: This function must only be called from the view's loop.
 *
 * @param view: The view.
 * @param scn: The new scene to render.
 * @return: The old scene.
 */
yf_scene_t *yf_view_swap(yf_view_t *view, yf_scene_t *scn);

/**
 * Renders a given scene in a view.
 *
 * NOTE: 'view_loop()' implicitly calls this function once per frame.
 *
 * @param view: The view.
 * @param scn: The scene to render.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_view_render(yf_view_t *view, yf_scene_t *scn);

/**
 * Deinitializes a view.
 *
 * @param view: The view to deinitialize. Can be 'NULL'.
 */
void yf_view_deinit(yf_view_t *view);

YF_DECLS_END

#endif /* YF_YF_VIEW_H */
