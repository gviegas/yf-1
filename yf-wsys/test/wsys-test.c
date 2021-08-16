/*
 * YF
 * wsys-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include "test.h"

int yf_test_window(void);
int yf_test_event(void);

static const char *l_ids[] = {
    "window",
    "event"
};

static int (*l_fns[])(void) = {
    yf_test_window,
    yf_test_event
};

_Static_assert(sizeof l_ids / sizeof *l_ids == sizeof l_fns / sizeof *l_fns,
               "!sizeof");

const YF_test yf_g_test = {
    .name = "wsys",
    .ids = l_ids,
    .fns = l_fns,
    .n = sizeof l_ids / sizeof *l_ids
};
