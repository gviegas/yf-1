/*
 * YF
 * core-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include "test.h"

int yf_test_context(void);
int yf_test_draw(void);

static const char *l_ids[] = {
    "context",
    "draw"
};

static int (*l_fns[])(void) = {
    yf_test_context,
    yf_test_draw
};

_Static_assert(sizeof l_ids / sizeof *l_ids == sizeof l_fns / sizeof *l_fns,
               "!sizeof");

const YF_test yf_g_test = {
    .name = "core",
    .ids = l_ids,
    .fns = l_fns,
    .n = sizeof l_ids / sizeof *l_ids
};
