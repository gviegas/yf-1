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
  void *(*init)(unsigned, unsigned, const char *, unsigned);
  int (*open)(void *);
  int (*close)(void *);
  int (*resize)(void *, unsigned, unsigned);
  int (*toggle)(void *);
  int (*settitle)(void *, const char *);
  void (*getsize)(void *, unsigned *, unsigned *);
  unsigned (*getwidth)(void *);
  unsigned (*getheight)(void *);
  void (*deinit)(void *);
} YF_win_imp;

/* Gets the window implementation. */
const YF_win_imp *yf_getwinimp(void);

#endif /* YF_WINDOW_H */
