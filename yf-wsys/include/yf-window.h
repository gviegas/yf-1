/*
 * YF
 * yf-window.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_WINDOW_H
#define YF_YF_WINDOW_H

#include <yf/com/yf-defs.h>

YF_DECLS_BEGIN

/* Opaque type defining a window. */
typedef struct YF_window_o *YF_window;

/* Window creation flags. */
#define YF_WINCREAT_FULLSCREEN 0x01
#define YF_WINCREAT_BORDERLESS 0x02
#define YF_WINCREAT_RESIZABLE  0x04
#define YF_WINCREAT_HIDDEN     0x08

/* Initializes a new window. */
YF_window yf_window_init(unsigned width, unsigned height, const char *title,
    unsigned creat_mask);

/* Opens a window. */
int yf_window_open(YF_window win);

/* Closes a window. */
int yf_window_close(YF_window win);

/* Resizes a window. */
int yf_window_resize(YF_window win, unsigned width, unsigned height);

/* Toggles fullscreen mode. */
int yf_window_toggle(YF_window win);

/* Sets window title. */
int yf_window_settitle(YF_window win, const char *title);

/* Gets window size. */
void yf_window_getsize(YF_window win, unsigned *width, unsigned *height);

/* Deinitializes a window. */
void yf_window_deinit(YF_window win);

YF_DECLS_END

#endif /* YF_YF_WINDOW_H */
