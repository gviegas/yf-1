/*
 * YF
 * test-wsi.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"
#include "yf-wsi.h"

/* Tests wsi. */
int yf_test_wsi(void)
{
    yf_window_t *win = yf_window_init(480, 300, "test-wsi", YF_WINCREAT_HIDDEN);
    assert(win != NULL);

    yf_context_t *ctx = yf_context_init();
    assert(ctx != NULL);

    YF_TEST_PRINT("init", "win", "wsi");
    yf_wsi_t *wsi = yf_wsi_init(ctx, win);
    if (wsi == NULL)
        return -1;

    yf_image_t *const *imgs;
    unsigned n;

    YF_TEST_PRINT("getimages", "wsi, &n", "");
    imgs = yf_wsi_getimages(wsi, &n);
    if (imgs == NULL || n == 0)
        return -1;

    YF_TEST_PRINT("getlimit", "wsi", "");
    if (yf_wsi_getlimit(wsi) == 0)
        return -1;

    int idx;

    YF_TEST_PRINT("next", "wsi, 1", "idx");
    while ((idx = yf_wsi_next(wsi, 1)) < 0)
        ;

    YF_TEST_PRINT("present", "wsi, idx", "");
    if (yf_wsi_present(wsi, idx) != 0)
        return -1;

    YF_TEST_PRINT("next", "wsi, 0", "idx");
    if ((idx = yf_wsi_next(wsi, 0)) < 0)
        return -1;

    YF_TEST_PRINT("present", "wsi, idx", "");
    if (yf_wsi_present(wsi, idx) != 0)
        return -1;

    YF_TEST_PRINT("deinit", "wsi", "");
    yf_wsi_deinit(wsi);

    yf_context_deinit(ctx);
    yf_window_deinit(win);
    return 0;
}
