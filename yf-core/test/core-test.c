/*
 * YF
 * core-test.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include "test.h"

int yf_test_context(void);
int yf_test_buffer(void);
int yf_test_image(void);
int yf_test_pass(void);
int yf_test_stage(void);
int yf_test_dtable(void);
int yf_test_gstate(void);
int yf_test_cstate(void);
int yf_test_cmdbuf(void);
int yf_test_wsi(void);
int yf_test_draw(void);

static const char *ids_[] = {
    "context",
    "buffer",
    "image",
    "pass",
    "stage",
    "dtable",
    "gstate",
    "cstate",
    "cmdbuf",
    "wsi",
    "draw"
};

static int (*fns_[])(void) = {
    yf_test_context,
    yf_test_buffer,
    yf_test_image,
    yf_test_pass,
    yf_test_stage,
    yf_test_dtable,
    yf_test_gstate,
    yf_test_cstate,
    yf_test_cmdbuf,
    yf_test_wsi,
    yf_test_draw
};

_Static_assert(sizeof ids_ / sizeof *ids_ == sizeof fns_ / sizeof *fns_,
               "!sizeof");

const yf_test_t yf_g_test = {
    .name = "core",
    .ids = ids_,
    .fns = fns_,
    .n = sizeof ids_ / sizeof *ids_
};
