/*
 * YF
 * com-test.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include "test.h"

int yf_test_error(void);
int yf_test_clock(void);
int yf_test_list(void);
int yf_test_dict(void);
int yf_test_pubsub(void);

static const char *ids_[] = {
    "error",
    "clock",
    "list",
    "dict",
    "pubsub"
};

static int (*fns_[])(void) = {
    yf_test_error,
    yf_test_clock,
    yf_test_list,
    yf_test_dict,
    yf_test_pubsub
};

_Static_assert(sizeof ids_ / sizeof *ids_ == sizeof fns_ / sizeof *fns_,
               "!sizeof");

const yf_test_t yf_g_test = {
    .name = "com",
    .ids = ids_,
    .fns = fns_,
    .n = sizeof ids_ / sizeof *ids_
};
