/*
 * YF
 * coreobj.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef __STDC_NO_ATOMICS__
# include <stdatomic.h>
#else
# error "C11 atomics required"
#endif

#include "coreobj.h"
#include "error.h"

/* TODO: Thread-safe. */

/* Context instance. */
static YF_context l_ctx = NULL;

/* Pass instance (managed somewhere else). */
extern YF_pass yf_g_pass;

/* Unsets scene shared variables (defined somewhere else). */
void yf_unsetscn(void);

/* Flag stating whether or not the exit handler was installed already. */
static int l_installed = 0;

/* Exits with failure. */
static _Noreturn void exit_fatal(const char *info);

/* Sets the exit handler. */
static int set_handler(void);

/* Handles deinitialization before exiting. */
static void handle_exit(void);

YF_context yf_getctx(void)
{
  static atomic_flag flag = ATOMIC_FLAG_INIT;

  if (atomic_flag_test_and_set(&flag)) {
    while (l_ctx == NULL)
      ;
  } else if (l_ctx == NULL) {
    set_handler();
    if ((l_ctx = yf_context_init()) == NULL)
      exit_fatal(__func__);
  }

  return l_ctx;
}

YF_pass yf_getpass(void)
{
  if (yf_g_pass == NULL)
    exit_fatal(__func__);
  return yf_g_pass;
}

static _Noreturn void exit_fatal(const char *info)
{
#ifndef YF_DEVEL
  yf_printerr();
#endif
  printf("\n[YF] Fatal: Could not initialize core object.\n(%s)\n", info);
  exit(EXIT_FAILURE);
}

static int set_handler(void)
{
  if (!l_installed) {
    if (atexit(handle_exit) != 0)
      return -1;
    l_installed = 1;
  }
  return 0;
}

static void handle_exit(void)
{
  yf_unsetscn();
  yf_pass_deinit(yf_g_pass);
  yf_context_deinit(l_ctx);
}
