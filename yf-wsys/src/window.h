/*
 * YF
 * window.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_WINDOW_H
#define YF_WINDOW_H

#include "yf-window.h"

/* Type defining a specific window implementation. */
typedef struct {
  /* Implementations return a pointer to their internal data on 'init',
     which is then provided as parameter when calling other window functions.
     The 'wrapper' object is the client interface to this internal data. */
  void *(*init)(unsigned width, unsigned height, const char *title,
      unsigned creat_mask, YF_window wrapper);

  int (*open)(void *win);
  int (*close)(void *win);
  int (*resize)(void *win, unsigned width, unsigned height);
  int (*toggle)(void *win);
  int (*settitle)(void *win, const char *title);
  void (*getsize)(void *win, unsigned *width, unsigned *height);
  void (*deinit)(void *win);
} YF_win_imp;

/* Gets the window implementation. */
void yf_getwinimp(YF_win_imp *imp);

#endif /* YF_WINDOW_H */
