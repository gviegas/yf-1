/*
 * YF
 * test-image.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"
#include "yf-image.h"

/* Tests image. */
int yf_test_image(void)
{
    yf_context_t *ctx = yf_context_init();
    assert(ctx != NULL);

    yf_dim3_t dim = {1024, 1024, 1};

    YF_TEST_PRINT("init", "PIXFMT_RGBA8UNORM, {1024,1024,1}, 1, 1, 1", "img");
    yf_image_t *img = yf_image_init(ctx, YF_PIXFMT_RGBA8UNORM, dim, 1, 1, 1);
    if (img == NULL)
        return -1;

    dim.height = 768;

    YF_TEST_PRINT("init", "PIXFMT_D16UNORM, {1024,768,1}, 1, 1, 1", "img2");
    yf_image_t *img2 = yf_image_init(ctx, YF_PIXFMT_D16UNORM, dim, 1, 1, 1);
    if (img2 == NULL)
        return -1;

    int pixfmt;
    unsigned layers, levels, samples;

    YF_TEST_PRINT("getval", "img, &pixfmt, &dim, &levels, &layers, &samples",
                  "");
    yf_image_getval(img, &pixfmt, &dim, &levels, &layers, &samples);
    if (pixfmt != YF_PIXFMT_RGBA8UNORM ||
        dim.width != 1024 || dim.height != 1024 || dim.depth != 1 ||
        layers != 1 || levels != 1 || samples != 1)
        return -1;

    YF_TEST_PRINT("getval", "img2, &pixfmt, &dim, &levels, &layers, &samples",
                  "");
    yf_image_getval(img2, &pixfmt, &dim, &levels, &layers, &samples);
    if (pixfmt != YF_PIXFMT_D16UNORM ||
        dim.width != 1024 || dim.height != 768 || dim.depth != 1 ||
        layers != 1 || levels != 1 || samples != 1)
        return -1;

    unsigned char data[64*64*4] = {0};
    yf_off3_t off = {0};
    dim.width = dim.height = 64;

    YF_TEST_PRINT("copy", "img, {0,0,0}, {64,64,1}, 0, 0, data", "");
    if (yf_image_copy(img, off, dim, 0, 0, data) != 0)
        return -1;

    YF_TEST_PRINT("copy", "img2, {0,0,0}, {64,64,1}, 0, 0, data", "");
    if (yf_image_copy(img2, off, dim, 0, 0, data) != 0)
        return -1;

    YF_TEST_PRINT("copy", "img, {0,0,0}, {64,64,1}, 1, 0, data", "");
    if (yf_image_copy(img, off, dim, 1, 0, data) == 0)
        return -1;

    off.x = 1000;

    YF_TEST_PRINT("copy", "img, {1000,0,0}, {64,64,1}, 0, 0, data", "");
    if (yf_image_copy(img, off, dim, 0, 0, data) == 0)
        return -1;

    off.x = 0;
    off.z = 1;

    YF_TEST_PRINT("copy", "img2, {0,0,1}, {64,64,1}, 0, 0, data", "");
    if (yf_image_copy(img2, off, dim, 0, 0, data) == 0)
        return -1;

    YF_TEST_PRINT("deinit", "img2", "");
    yf_image_deinit(img2);

    YF_TEST_PRINT("deinit", "img", "");
    yf_image_deinit(img);

    yf_context_deinit(ctx);
    return 0;
}
