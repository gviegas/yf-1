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

/* Context instance. */
static YF_context l_ctx = NULL;

/* Pass instance (managed somewhere else). */
extern YF_pass yf_g_pass;

/* Unsets shared scene variables (defined somewhere else). */
void yf_unsetscn(void);

/* Handles deinitialization before exiting. */
static void handle_exit(void);

/* Exits with failure. */
static _Noreturn void exit_fatal(const char *info);

YF_context yf_getctx(void)
{
    static atomic_flag flag = ATOMIC_FLAG_INIT;

    if (atomic_flag_test_and_set(&flag)) {
        while (l_ctx == NULL)
            ;
    } else if (l_ctx == NULL) {
        if (atexit(handle_exit) != 0 || (l_ctx = yf_context_init()) == NULL)
            exit_fatal(__func__);
    }

    return l_ctx;
}

YF_pass yf_getpass(void)
{
    static atomic_flag flag = ATOMIC_FLAG_INIT;

    if (atomic_flag_test_and_set(&flag)) {
        while (yf_g_pass == NULL)
            ;
    } else if (yf_g_pass == NULL) {
        exit_fatal(__func__);
    }

    return yf_g_pass;
}

static void handle_exit(void)
{
    yf_unsetscn();
    yf_pass_deinit(yf_g_pass);
    yf_context_deinit(l_ctx);
}

static _Noreturn void exit_fatal(const char *info)
{
#ifndef YF_DEVEL
    yf_printerr();
#endif

    fprintf(stderr, "\n[YF] Fatal:\n! Failed to initialize core object");
    if (info != NULL)
        fprintf(stderr, "\n! %s\n\n", info);
    else
        fprintf(stderr, "\n\n");

    exit(EXIT_FAILURE);
}
