/*
 * YF
 * coreobj.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>

#include "coreobj.h"
#include "error.h"

/* Context instance. */
static YF_context l_ctx = NULL;

/* Pass instance (managed somewhere else). */
extern YF_pass yf_g_pass;

/* Flag stating whether or not the exit handler was installed already. */
static int l_installed = 0;

/* Exits with failure. */
static _Noreturn void exit_fatal(const char *info);

/* Sets the exit handler. */
static int set_handler(void);

/* Handles deinitialization before exiting. */
static void handle_exit(void);

YF_context yf_getctx(void) {
  if (l_ctx == NULL) {
    if ((l_ctx = yf_context_init()) == NULL)
      exit_fatal(__func__);
    else
      set_handler();
  }
  return l_ctx;
}

YF_pass yf_getpass(void) {
  if (yf_g_pass == NULL)
    exit_fatal(__func__);
  return yf_g_pass;
}

static _Noreturn void exit_fatal(const char *info) {
#ifndef YF_DEBUG
  yf_printerr();
#endif
  printf("\n[YF] Fatal: Could not initialize core object.\n(%s)\n", info);
  exit(EXIT_FAILURE);
}

static int set_handler(void) {
  if (!l_installed) {
    if (atexit(handle_exit) != 0)
      return -1;
    l_installed = 1;
  }
  return 0;
}

static void handle_exit(void) {
  yf_pass_deinit(yf_g_pass);
  yf_g_pass = NULL;
  yf_context_deinit(l_ctx);
  l_ctx = NULL;
}
