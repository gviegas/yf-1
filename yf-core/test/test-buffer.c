/*
 * YF
 * test-buffer.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"
#include "yf-buffer.h"

/* Tests buffer. */
int yf_test_buffer(void)
{
    yf_context_t *ctx = yf_context_init();
    assert(ctx != NULL);

    YF_TEST_PRINT("init", "2048", "buf");
    yf_buffer_t *buf = yf_buffer_init(ctx, 2048);
    if (buf == NULL)
        return -1;

    YF_TEST_PRINT("getsize", "buf", "");
    if (yf_buffer_getsize(buf) != 2048)
        return -1;

    YF_TEST_PRINT("init", "1048576", "buf2");
    yf_buffer_t *buf2 = yf_buffer_init(ctx, 1048576);
    if (buf2 == NULL)
        return -1;

    YF_TEST_PRINT("getsize", "buf2", "");
    if (yf_buffer_getsize(buf2) != 1048576)
        return -1;

    unsigned char data[4096] = {0};

    YF_TEST_PRINT("copy", "buf, 0, data, 2048", "");
    if (yf_buffer_copy(buf, 0, data, 2048) != 0)
        return -1;

    YF_TEST_PRINT("copy", "buf2, 0, data, 4096", "");
    if (yf_buffer_copy(buf2, 0, data, 4096) != 0)
        return -1;

    YF_TEST_PRINT("copy", "buf, 0, data, 2049", "");
    if (yf_buffer_copy(buf, 0, data, 2049) == 0)
        return -1;

    YF_TEST_PRINT("copy", "buf2, 1048000, data, 1024", "");
    if (yf_buffer_copy(buf2, 1048000, data, 1024) == 0)
        return -1;

    YF_TEST_PRINT("deinit", "buf2", "");
    yf_buffer_deinit(buf2);

    YF_TEST_PRINT("deinit", "buf", "");
    yf_buffer_deinit(buf);

    yf_context_deinit(ctx);
    return 0;
}
