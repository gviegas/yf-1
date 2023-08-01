/*
 * YF
 * test-pass.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"
#include "yf-pass.h"

/* Tests pass. */
int yf_test_pass(void)
{
    yf_context_t *ctx = yf_context_init();
    assert(ctx != NULL);

    const yf_dim3_t dim = {1024, 768, 1};

    yf_image_t *color, *resolve, *depth;

    color = yf_image_init(ctx, YF_PIXFMT_RGBA8UNORM, dim, 1, 1, 8);
    assert(color != NULL);

    resolve = yf_image_init(ctx, YF_PIXFMT_RGBA8UNORM, dim, 1, 1, 1);
    assert(resolve != NULL);

    depth = yf_image_init(ctx, YF_PIXFMT_D16UNORM, dim, 1, 1, 1);
    assert(depth != NULL);

    yf_colordsc_t clr_dsc = {
        .pixfmt = YF_PIXFMT_RGBA8UNORM,
        .samples = 8,
        .loadop = YF_LOADOP_UNDEF,
        .storeop = YF_STOREOP_UNDEF
    };

    yf_colordsc_t rsv_dsc = {
        .pixfmt = YF_PIXFMT_RGBA8UNORM,
        .samples = 1,
        .loadop = YF_LOADOP_UNDEF,
        .storeop = YF_STOREOP_STORE
    };

    yf_depthdsc_t dep_dsc = {
        .pixfmt = YF_PIXFMT_D16UNORM,
        .samples = 1,
        .depth_loadop = YF_LOADOP_UNDEF,
        .depth_storeop = YF_STOREOP_STORE,
        .stencil_loadop = YF_LOADOP_UNDEF,
        .stencil_storeop = YF_STOREOP_UNDEF
    };

    YF_TEST_PRINT("init", "&clr_dsc, 1, &rsv_dsc, &dep_dsc", "pass");
    yf_pass_t *pass = yf_pass_init(ctx, &clr_dsc, 1, &rsv_dsc, &dep_dsc);
    if (pass == NULL)
        return -1;

    yf_attach_t clr_att = {
        .img = color,
        .layer_base = 0
    };

    yf_attach_t rsv_att = {
        .img = resolve,
        .layer_base = 0
    };

    yf_attach_t dep_att = {
        .img = depth,
        .layer_base = 0
    };

    const yf_dim2_t dim2 = {dim.width, dim.height};

    YF_TEST_PRINT("maketarget",
                  "pass, {1024,768}, 1, &clr_att, &rsv_att, &dep_att", "tgt");
    yf_target_t *tgt = yf_pass_maketarget(pass, dim2, 1, &clr_att, &rsv_att,
                                          &dep_att);
    if (tgt == NULL)
        return -1;

    YF_TEST_PRINT("unmktarget", "pass, tgt", "");
    yf_pass_unmktarget(pass, tgt);

    YF_TEST_PRINT("deinit", "pass", "");
    yf_pass_deinit(pass);

    yf_image_deinit(depth);
    yf_image_deinit(resolve);
    yf_image_deinit(color);
    yf_context_deinit(ctx);
    return 0;
}
