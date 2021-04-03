/*
 * YF
 * yf-window.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_WINDOW_H
#define YF_YF_WINDOW_H

#include "yf/com/yf-defs.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a window.
 */
typedef struct YF_window_o *YF_window;

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
YF_window yf_window_init(unsigned width, unsigned height, const char *title,
    unsigned creat_mask);

/**
 * Opens a window.
 *
 * @param win: The window.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_window_open(YF_window win);

/**
 * Closes a window.
 *
 * @param win: The window.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_window_close(YF_window win);

/**
 * Resizes a window.
 *
 * @param win: The window.
 * @param width: The new width.
 * @param height: The new height.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_window_resize(YF_window win, unsigned width, unsigned height);

/**
 * Toggles fullscreen mode.
 *
 * @param win: The window.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_window_toggle(YF_window win);

/**
 * Sets the title of a window.
 *
 * @param win: The window.
 * @param title: The new title.
 * @return: On success, returns zero. Otherwise, 'NULL' is returned and the
 *  global error is set to indicate the cause.
 */
int yf_window_settitle(YF_window win, const char *title);

/**
 * Gets the size of a window.
 *
 * @param win: The window.
 * @param width: The destination for the window's width.
 * @param height: The destination for the window's height.
 */
void yf_window_getsize(YF_window win, unsigned *width, unsigned *height);

/**
 * Deinitializes a window.
 *
 * @param win: The window to deinitialize. Can be 'NULL'.
 */
void yf_window_deinit(YF_window win);

YF_DECLS_END

#endif /* YF_YF_WINDOW_H */
