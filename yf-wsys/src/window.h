/*
 * YF
 * window.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_WINDOW_H
#define YF_WINDOW_H

#include "yf-window.h"

/* Specific window implementation. */
typedef struct yf_win_imp {
    /* Implementations return a pointer to their internal data on 'init()',
       which is then provided as parameter when calling other window functions.
       The 'wrapper' object is the client interface to this internal data. */
    void *(*init)(unsigned width, unsigned height, const char *title,
                  unsigned creat_mask, yf_window_t *wrapper);

    int (*open)(void *win);
    int (*close)(void *win);
    int (*resize)(void *win, unsigned width, unsigned height);
    int (*toggle)(void *win);
    int (*settitle)(void *win, const char *title);
    void (*getsize)(void *win, unsigned *width, unsigned *height);
    void (*deinit)(void *win);
} yf_win_imp_t;

/* Gets the window implementation. */
void yf_getwinimp(yf_win_imp_t *imp);

#endif /* YF_WINDOW_H */
