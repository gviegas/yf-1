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
  void *(*init)(unsigned w, unsigned h, const char *title, unsigned creat);
  int (*open)(void *win);
  int (*close)(void *win);
  int (*resize)(void *win, unsigned w, unsigned h);
  int (*toggle)(void *win);
  int (*settitle)(void *win, const char *title);
  void (*getsize)(void *win, unsigned *w, unsigned *h);
  void (*deinit)(void *win);
} YF_win_imp;

/* Gets the window implementation. */
void yf_getwinimp(YF_win_imp *imp);

#endif /* YF_WINDOW_H */
