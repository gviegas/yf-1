/*
 * YF
 * platform-xcb.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>

#include "platform-xcb.h"
#include "platform.h"

/* Window implementation functions. */
static void *init_win(unsigned, unsigned, const char *, unsigned);
static int open_win(void *);
static int close_win(void *);
static int resize_win(void *, unsigned, unsigned);
static int toggle_win(void *);
static int settitle_win(void *, const char *);
static void getsize_win(void *, unsigned *, unsigned *);
static void deinit_win(void *);

const YF_win_imp yf_g_winxcb = {
  .init = init_win,
  .open = open_win,
  .close = close_win,
  .resize = resize_win,
  .toggle = toggle_win,
  .settitle = settitle_win,
  .getsize = getsize_win,
  .deinit = deinit_win
};

/* Event implementation functions. */
static int poll_evt(unsigned);
static void changed_evt(int);

const YF_evt_imp yf_g_evtxcb = {.poll = poll_evt, .changed = changed_evt};

/* Variables are set by 'yf_loadxcb' and unset by 'yf_unldxcb'. */
YF_xcbvars yf_g_xcbvars = {0};

/* Shared object handle. */
static void * l_handle = NULL;

int yf_loadxcb(void) {
  if (l_handle != NULL)
    return 0;

  /* TODO */
  return -1;
}

void yf_unldxcb(void) {
  if (l_handle != NULL) {
    dlclose(l_handle);
    l_handle = NULL;
    memset(&yf_g_xcbvars, 0, sizeof yf_g_xcbvars);
  }
}

static void *init_win(unsigned width, unsigned height, const char *title,
    unsigned creat_mask)
{
  /* TODO */
  return NULL;
}

static int open_win(void *win) {
  assert(win != NULL);
  /* TODO */
  return -1;
}

static int close_win(void *win) {
  assert(win != NULL);
  /* TODO */
  return -1;
}

static int resize_win(void *win, unsigned width, unsigned height) {
  assert(win != NULL);
  /* TODO */
  return -1;
}

static int toggle_win(void *win) {
  assert(win != NULL);
  /* TODO */
  return -1;
}

static int settitle_win(void *win, const char *title) {
  assert(win != NULL);
  /* TODO */
  return -1;
}

static void getsize_win(void *win, unsigned *width, unsigned *height) {
  assert(win != NULL);
  /* TODO */
}

static void deinit_win(void *win) {
  /* TODO */
}

static int poll_evt(unsigned evt_mask) {
  /* TODO */
  return -1;
}

static void changed_evt(int evt) {
  /* TODO */
}
