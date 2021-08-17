/*
 * YF
 * test-stage.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"
#include "yf-stage.h"

#define YF_VERTSHD "tmp/vert"
#define YF_FRAGSHD "tmp/frag"

/* Tests stage. */
int yf_test_stage(void)
{
    YF_context ctx = yf_context_init();
    assert(ctx != NULL);

    YF_shdid vert, frag;

    YF_TEST_PRINT("loadshd", YF_VERTSHD", &vert", "");
    if (yf_loadshd(ctx, YF_VERTSHD, &vert) != 0)
        return -1;

    YF_TEST_PRINT("loadshd", YF_FRAGSHD", &frag", "");
    if (yf_loadshd(ctx, YF_FRAGSHD, &frag) != 0)
        return -1;

    YF_TEST_PRINT("unldshd", "frag", "");
    yf_unldshd(ctx, frag);

    YF_TEST_PRINT("unldshd", "vert", "");
    yf_unldshd(ctx, vert);

    yf_context_deinit(ctx);
    return 0;
}
