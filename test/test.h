/*
 * YF
 * test.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_TEST_H
#define YF_TEST_H

#include <stddef.h>

#define YF_TEST_ALL "all"

/* Type defining test(s) to execute. */
typedef struct {
    char name[64];
    const char *const *ids;
    int (*const *fns)(void);
    size_t n;
} YF_test;

extern const YF_test yf_g_test;

#endif /* YF_TEST_H */
