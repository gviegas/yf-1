/*
 * YF
 * yf-window.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_WINDOW_H
#define YF_YF_WINDOW_H

#include "yf/com/yf-defs.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a window.
 */
typedef struct yf_window yf_window_t;

/**
 * Window creation flags.
 */
#define YF_WINCREAT_FULLSCREEN 0x01
#define YF_WINCREAT_BORDERLESS 0x02
#define YF_WINCREAT_RESIZABLE  0x04
#define YF_WINCREAT_HIDDEN     0x08

/**
 * Initializes a new window.
 *
 * @param width: The width of the window.
 * @param height: The height of the window.
 * @param title: The title of the window.
 * @param creat_mask: A mask of 'YF_WINCREAT' values indicating creation-time
 *  properties.
 * @return: On success, returns a new window. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_window_t *yf_window_init(unsigned width, unsigned height, const char *title,
                            unsigned creat_mask);

/**
 * Opens a window.
 *
 * @param win: The window.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_window_open(yf_window_t *win);

/**
 * Closes a window.
 *
 * @param win: The window.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_window_close(yf_window_t *win);

/**
 * Resizes a window.
 *
 * @param win: The window.
 * @param width: The new width.
 * @param height: The new height.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_window_resize(yf_window_t *win, unsigned width, unsigned height);

/**
 * Toggles fullscreen mode.
 *
 * @param win: The window.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_window_toggle(yf_window_t *win);

/**
 * Sets the title of a window.
 *
 * @param win: The window.
 * @param title: The new title.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_window_settitle(yf_window_t *win, const char *title);

/**
 * Gets the size of a window.
 *
 * @param win: The window.
 * @param width: The destination for the window's width.
 * @param height: The destination for the window's height.
 */
void yf_window_getsize(yf_window_t *win, unsigned *width, unsigned *height);

/**
 * Deinitializes a window.
 *
 * @param win: The window to deinitialize. Can be 'NULL'.
 */
void yf_window_deinit(yf_window_t *win);

YF_DECLS_END

#endif /* YF_YF_WINDOW_H */
