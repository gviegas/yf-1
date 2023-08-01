/*
 * YF
 * test-cmdbuf.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"
#include "yf-cmdbuf.h"

/* Tests cmdbuf. */
int yf_test_cmdbuf(void)
{
    yf_context_t *ctx = yf_context_init();
    assert(ctx != NULL);

    YF_TEST_PRINT("get", "CMDBUF_GRAPH", "graph_cb");
    yf_cmdbuf_t *graph_cb = yf_cmdbuf_get(ctx, YF_CMDBUF_GRAPH);
    if (graph_cb == NULL)
        return -1;

    YF_TEST_PRINT("get", "CMDBUF_COMP", "comp_cb");
    yf_cmdbuf_t *comp_cb = yf_cmdbuf_get(ctx, YF_CMDBUF_COMP);
    if (comp_cb == NULL)
        return -1;

    YF_TEST_PRINT("get", "CMDBUF_XFER", "xfer_cb");
    yf_cmdbuf_t *xfer_cb = yf_cmdbuf_get(ctx, YF_CMDBUF_XFER);
    if (xfer_cb == NULL)
        return -1;

    YF_TEST_PRINT("end", "xfer_cb", "");
    if (yf_cmdbuf_end(xfer_cb) != 0)
        return -1;

    YF_TEST_PRINT("end", "comp_cb", "");
    if (yf_cmdbuf_end(comp_cb) != 0)
        return -1;

    YF_TEST_PRINT("end", "graph_cb", "");
    if (yf_cmdbuf_end(graph_cb) != 0)
        return -1;

    YF_TEST_PRINT("exec", "", "");
    if (yf_cmdbuf_exec(ctx) != 0)
        return -1;

    yf_context_deinit(ctx);
    return 0;
}
