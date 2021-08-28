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
        .dim = {16, 16}
    };

    YF_TEST_PRINT("initdt", "&data", "tex");
    YF_texture tex = yf_texture_initdt(&data);
    if (tex == NULL)
        return -1;

    yf_print_tex(tex);
    yf_print_tex(NULL);

    YF_TEST_PRINT("initdt", "&data", "tex2");
    YF_texture tex2 = yf_texture_initdt(&data);
    if (tex2 == NULL)
        return -1;

    yf_print_tex(tex2);
    yf_print_tex(NULL);

    YF_TEST_PRINT("initdt", "&data", "tex3");
    YF_texture tex3 = yf_texture_initdt(&data);
    if (tex3 == NULL)
        return -1;

    yf_print_tex(tex3);
    yf_print_tex(NULL);

    data.dim.width = 24;
    printf("- dim. changed -\n");

    YF_TEST_PRINT("initdt", "&data", "tex4");
    YF_texture tex4 = yf_texture_initdt(&data);
    if (tex4 == NULL)
        return -1;

    yf_print_tex(tex4);
    yf_print_tex(NULL);

    YF_TEST_PRINT("initdt", "&data", "tex5");
    YF_texture tex5 = yf_texture_initdt(&data);
    if (tex5 == NULL)
        return -1;

    yf_print_tex(tex5);
    yf_print_tex(NULL);

    data.pixfmt = YF_PIXFMT_BGRA8SRGB;
    printf("- pixfmt changed -\n");

    YF_TEST_PRINT("initdt", "&data", "tex6");
    YF_texture tex6 = yf_texture_initdt(&data);
    if (tex6 == NULL)
        return -1;

    yf_print_tex(tex6);
    yf_print_tex(NULL);

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
