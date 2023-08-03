/*
 * YF
 * coreobj.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
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
static yf_context_t *ctx_ = NULL;

/* Pass instance (managed elsewhere). */
/* TODO: Remove. */
extern yf_pass_t *yf_g_pass;

/* Unsets shared variables (defined elsewhere). */
void yf_unsetscn(void);
void yf_unsetmesh(void);
void yf_unsettex(void);

/* Handles deinitialization before exiting. */
static void handle_exit(void)
{
    yf_unsetscn();
    yf_unsetmesh();
    yf_unsettex();
    yf_pass_deinit(yf_g_pass);
    yf_context_deinit(ctx_);
}

/* Exits with failure. */
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

yf_context_t *yf_getctx(void)
{
    static atomic_flag flag = ATOMIC_FLAG_INIT;

    if (atomic_flag_test_and_set(&flag)) {
        while (ctx_ == NULL)
            ;
    } else if (ctx_ == NULL) {
        if (atexit(handle_exit) != 0 || (ctx_ = yf_context_init()) == NULL)
            exit_fatal(__func__);
    }

    return ctx_;
}

yf_pass_t *yf_getpass(void)
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
