/*
 * YF
 * test-texture.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "test.h"
#include "print.h"
#include "texture.h"

/* Tests texture. */
/* TODO: More tests. */
int yf_test_texture(void)
{
    unsigned char bytes[64*64*4];

    YF_texdt data = {
        .data = bytes,
        .pixfmt = YF_PIXFMT_RGBA8UNORM,
        .dim = {16, 16},
        .splr = {{YF_WRAPMODE_CLAMP}, {YF_FILTER_LINEAR}},
        .uvset = YF_UVSET_1
    };

    YF_TEST_PRINT("init", "&data", "tex");
    YF_texture tex = yf_texture_init(&data);
    if (tex == NULL)
        return -1;

    yf_print_tex(tex);
    yf_print_tex(NULL);

    YF_TEST_PRINT("getdim", "tex", "");
    YF_dim2 dim = yf_texture_getdim(tex);
    if (dim.width != data.dim.width || dim.height != data.dim.height)
        return -1;

    YF_TEST_PRINT("init", "&data", "tex2");
    YF_texture tex2 = yf_texture_init(&data);
    if (tex2 == NULL)
        return -1;

    yf_print_tex(tex2);
    yf_print_tex(NULL);

    YF_TEST_PRINT("getsplr", "tex2", "");
    YF_sampler splr = *yf_texture_getsplr(tex2);
    if (splr.wrapmode.u != data.splr.wrapmode.u ||
        splr.wrapmode.v != data.splr.wrapmode.v ||
        splr.wrapmode.w != data.splr.wrapmode.w ||
        splr.filter.mag != data.splr.filter.mag ||
        splr.filter.min != data.splr.filter.min ||
        splr.filter.mipmap != data.splr.filter.mipmap)
        return -1;

    YF_TEST_PRINT("init", "&data", "tex3");
    YF_texture tex3 = yf_texture_init(&data);
    if (tex3 == NULL)
        return -1;

    yf_print_tex(tex3);
    yf_print_tex(NULL);

    YF_TEST_PRINT("getuv", "tex3", "");
    if (yf_texture_getuv(tex3) != YF_UVSET_1)
        return -1;

    YF_TEST_PRINT("setuv", "tex3, UVSET_0", "");
    yf_texture_setuv(tex3, YF_UVSET_0);
    if (yf_texture_getuv(tex3) != YF_UVSET_0)
        return -1;

    yf_print_tex(tex3);

    data.dim.width = 24;
    printf("- dim. changed -\n");

    YF_TEST_PRINT("init", "&data", "tex4");
    YF_texture tex4 = yf_texture_init(&data);
    if (tex4 == NULL)
        return -1;

    yf_print_tex(tex4);
    yf_print_tex(NULL);

    YF_TEST_PRINT("getdim", "tex4", "");
    dim = yf_texture_getdim(tex4);
    if (dim.width != data.dim.width || dim.height != data.dim.height ||
        dim.width == yf_texture_getdim(tex3).width ||
        dim.height != yf_texture_getdim(tex3).height)
        return -1;

    YF_TEST_PRINT("init", "&data", "tex5");
    YF_texture tex5 = yf_texture_init(&data);
    if (tex5 == NULL)
        return -1;

    yf_print_tex(tex5);
    yf_print_tex(NULL);

    data.pixfmt = YF_PIXFMT_BGRA8SRGB;
    printf("- pixfmt changed -\n");

    YF_TEST_PRINT("init", "&data", "tex6");
    YF_texture tex6 = yf_texture_init(&data);
    if (tex6 == NULL)
        return -1;

    yf_print_tex(tex6);
    yf_print_tex(NULL);

    YF_TEST_PRINT("getsplr", "tex6", "");
    yf_texture_getsplr(tex6)->wrapmode.v = YF_WRAPMODE_CLAMP;
    yf_texture_getsplr(tex6)->filter.min = YF_FILTER_LINEAR;
    splr = *yf_texture_getsplr(tex6);
    if (splr.wrapmode.u != data.splr.wrapmode.u ||
        splr.wrapmode.v == data.splr.wrapmode.v ||
        splr.wrapmode.w != data.splr.wrapmode.w ||
        splr.filter.mag != data.splr.filter.mag ||
        splr.filter.min == data.splr.filter.min ||
        splr.filter.mipmap != data.splr.filter.mipmap)
        return -1;

    yf_print_tex(tex6);

    YF_TEST_PRINT("deinit", "tex4", "");
    yf_texture_deinit(tex4);

    yf_print_tex(NULL);

    YF_TEST_PRINT("deinit", "tex6", "");
    yf_texture_deinit(tex6);

    yf_print_tex(NULL);

    YF_TEST_PRINT("deinit", "tex2", "");
    yf_texture_deinit(tex2);

    yf_print_tex(NULL);

    YF_TEST_PRINT("deinit", "tex", "");
    yf_texture_deinit(tex);

    yf_print_tex(NULL);

    YF_TEST_PRINT("deinit", "tex3", "");
    yf_texture_deinit(tex3);

    yf_print_tex(NULL);

    YF_TEST_PRINT("deinit", "tex5", "");
    yf_texture_deinit(tex5);

    yf_print_tex(NULL);

    return 0;
}
