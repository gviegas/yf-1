/*
 * YF
 * com-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include "test.h"

int yf_test_error(void);
int yf_test_clock(void);
int yf_test_list(void);
int yf_test_dict(void);
int yf_test_pubsub(void);

static const char *l_ids[] = {
    "error",
    "clock",
    "list",
    "dict",
    "pubsub"
};

static int (*l_fns[])(void) = {
    yf_test_error,
    yf_test_clock,
    yf_test_list,
    yf_test_dict,
    yf_test_pubsub
};

_Static_assert(sizeof l_ids / sizeof *l_ids == sizeof l_fns / sizeof *l_fns,
               "!sizeof");

const YF_test yf_g_test = {
    .name = "com",
    .ids = l_ids,
    .fns = l_fns,
    .n = sizeof l_ids / sizeof *l_ids
};
