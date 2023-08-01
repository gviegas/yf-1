/*
 * YF
 * test-dtable.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"
#include "yf-dtable.h"

/* Tests dtable. */
int yf_test_dtable(void)
{
    yf_context_t *ctx = yf_context_init();
    assert(ctx);

    const unsigned char data[16384] = {0};
    const size_t sz = 16384;
    const yf_off3_t off = {0};
    const yf_dim3_t dim = {128, 128, 1};

    yf_buffer_t *buf = yf_buffer_init(ctx, sz);
    assert(buf);
    if (yf_buffer_copy(buf, off.x, data, sz) != 0)
        assert(0);

    yf_image_t *img = yf_image_init(ctx, YF_PIXFMT_R8UNORM, dim, 2, 1, 1);
    assert(img != NULL);
    if (yf_image_copy(img, off, dim, 0, 0, data) != 0 ||
        yf_image_copy(img, off, dim, 1, 0, data) != 0)
        assert(0);

    const yf_dentry_t entries[] = {
        {
            .binding = 0,
            .dtype = YF_DTYPE_UNIFORM,
            .elements = 1,
            .info = NULL
        },
        {
            .binding = 2,
            .dtype = YF_DTYPE_UNIFORM,
            .elements = 16,
            .info = NULL
        },
        {
            .binding = 3,
            .dtype = YF_DTYPE_MUTABLE,
            .elements = 1,
            .info = NULL
        },
        {
            .binding = 1,
            .dtype = YF_DTYPE_ISAMPLER,
            .elements = 10,
            .info = NULL
        }
    };
    const size_t entry_n = sizeof entries / sizeof *entries;

    YF_TEST_PRINT("init", "entries, entry_n", "dtb");
    yf_dtable_t *dtb = yf_dtable_init(ctx, entries, entry_n);
    if (dtb == NULL)
        return -1;

    YF_TEST_PRINT("alloc", "dtb, 2", "");
    if (yf_dtable_alloc(dtb, 2) != 0)
        return -1;

    yf_slice_t slice = {0, 16};

    yf_buffer_t *bufs[16];
    size_t offs[16];
    size_t szs[16];
    for (size_t i = 0; i < 16; i++) {
        bufs[i] = buf;
        offs[i] = i&1 ? 0 : 4096;
        szs[i] = 1024;
    }

    YF_TEST_PRINT("copybuf", "dtb, 0, 2, {0, 16}, bufs, offs, szs", "");
    if (yf_dtable_copybuf(dtb, 0, 2, slice, bufs, offs, szs) != 0)
        return -1;

    slice.n = 10;

    yf_image_t *imgs[10];
    unsigned lays[10];
    for (size_t i = 0; i < 10; i++) {
        imgs[i] = img;
        lays[i] = i&1 ? 0 : 1;
    }

    YF_TEST_PRINT("copyimg", "dtb, 1, 1, {0, 10}, imgs, lays, NULL", "");
    if (yf_dtable_copyimg(dtb, 1, 1, slice, imgs, lays, NULL) != 0)
        return -1;

    YF_TEST_PRINT("dealloc", "dtb", "");
    yf_dtable_dealloc(dtb);

    YF_TEST_PRINT("deinit", "dtb", "");
    yf_dtable_deinit(dtb);

    yf_image_deinit(img);
    yf_buffer_deinit(buf);
    yf_context_deinit(ctx);
    return 0;
}
