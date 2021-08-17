/*
 * YF
 * test-gstate.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"
#include "yf-gstate.h"

#define YF_VERTSHD "tmp/vert"

/* Tests gstate. */
int yf_test_gstate(void)
{
    YF_context ctx = yf_context_init();
    assert(ctx != NULL);

    const int pixfmt = YF_PIXFMT_BGRA8SRGB;
    const YF_dim3 dim = {800, 600, 1};
    YF_image img = yf_image_init(ctx, pixfmt, dim, 1, 1, 1);
    assert(img != NULL);

    const YF_colordsc dsc = {pixfmt, 1, YF_LOADOP_LOAD, YF_STOREOP_STORE};
    YF_pass pass = yf_pass_init(ctx, &dsc, 1, NULL, NULL);
    assert(pass != NULL);

    YF_stage stg = {YF_STAGE_VERT, 0, "main"};
    if (yf_loadshd(ctx, YF_VERTSHD, &stg.shd) != 0)
        assert(0);

    const YF_dentry entry = {YF_DTYPE_UNIFORM, 0, 1, NULL};
    YF_dtable dtb = yf_dtable_init(ctx, &entry, 1);
    assert(dtb != NULL);

    const YF_vattr attr = {0, YF_VFMT_FLOAT4, 0};
    const YF_vinput input = {&attr, 1, 0, YF_VRATE_VERT};

    const YF_gconf conf = {
        .pass = pass,
        .stgs = &stg,
        .stg_n = 1,
        .dtbs = &dtb,
        .dtb_n = 1,
        .vins = &input,
        .vin_n = 1,
        .primitive = YF_PRIMITIVE_TRIANGLE,
        .polymode = YF_POLYMODE_FILL,
        .cullmode = YF_CULLMODE_BACK,
        .winding = YF_WINDING_CCW
    };

    YF_TEST_PRINT("init", "&conf", "gst");
    YF_gstate gst = yf_gstate_init(ctx, &conf);
    if (gst == NULL)
        return -1;

    YF_TEST_PRINT("getpass", "gst", "");
    if (yf_gstate_getpass(gst) != pass)
        return -1;

    YF_TEST_PRINT("getshd", "gst, STAGE_VERT", "");
    if (yf_gstate_getstg(gst, YF_STAGE_VERT) == NULL)
        return -1;

    YF_TEST_PRINT("getshd", "gst, STAGE_FRAG", "");
    if (yf_gstate_getstg(gst, YF_STAGE_FRAG) != NULL)
        return -1;

    YF_TEST_PRINT("getdtb", "gst, 0", "");
    if (yf_gstate_getdtb(gst, 0) != dtb)
        return -1;

    YF_TEST_PRINT("deinit", "gst", "");
    yf_gstate_deinit(gst);

    yf_dtable_deinit(dtb);
    yf_unldshd(ctx, stg.shd);
    yf_pass_deinit(pass);
    yf_image_deinit(img);
    yf_context_deinit(ctx);
    return 0;
}
