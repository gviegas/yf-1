/*
 * YF
 * test-cstate.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"
#include "yf-cstate.h"

#define YF_COMPSHD "tmp/comp"

/* Tests cstate. */
int yf_test_cstate(void)
{
    yf_context_t *ctx = yf_context_init();
    assert(ctx != NULL);

    yf_shdid_t shd;
    if (yf_loadshd(ctx, YF_COMPSHD, &shd) != 0)
        assert(0);

    const yf_dentry_t entry = {YF_DTYPE_MUTABLE, 0, 1, NULL};
    yf_dtable_t *dtb = yf_dtable_init(ctx, &entry, 1);
    assert(dtb != NULL);

    const yf_cconf_t conf = {
        .stg = {YF_STAGE_COMP, shd, "main"},
        .dtbs = &dtb,
        .dtb_n = 1
    };

    YF_TEST_PRINT("init", "&conf", "cst");
    yf_cstate_t *cst = yf_cstate_init(ctx, &conf);
    if (cst == NULL)
        return -1;

    YF_TEST_PRINT("getstg", "cst", "");
    if (yf_cstate_getstg(cst) == NULL)
        return -1;

    YF_TEST_PRINT("getdtb", "cst, 0", "");
    if (yf_cstate_getdtb(cst, 0) != dtb)
        return -1;

    YF_TEST_PRINT("deinit", "cst", "");
    yf_cstate_deinit(cst);

    yf_dtable_deinit(dtb);
    yf_unldshd(ctx, shd);
    yf_context_deinit(ctx);
    return 0;
}
