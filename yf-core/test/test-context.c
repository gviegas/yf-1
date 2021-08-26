/*
 * YF
 * test-context.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "test.h"
#include "print.h"
#include "yf-context.h"

/* Tests context. */
int yf_test_context(void)
{
    YF_TEST_PRINT("init", "", "ctx");
    YF_context ctx = yf_context_init();
    if (ctx == NULL)
        return -1;

    yf_print_ctx(ctx);
    yf_print_lim(yf_getlimits(ctx));

    YF_TEST_PRINT("deinit", "ctx", "");
    yf_context_deinit(ctx);

    return 0;
}
