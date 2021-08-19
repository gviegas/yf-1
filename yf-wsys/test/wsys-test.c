/*
 * YF
 * wsys-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include "test.h"

int yf_test_window(void);
int yf_test_event(void);

static const char *ids_[] = {
    "window",
    "event"
};

static int (*fns_[])(void) = {
    yf_test_window,
    yf_test_event
};

_Static_assert(sizeof ids_ / sizeof *ids_ == sizeof fns_ / sizeof *fns_,
               "!sizeof");

const YF_test yf_g_test = {
    .name = "wsys",
    .ids = ids_,
    .fns = fns_,
    .n = sizeof ids_ / sizeof *ids_
};
