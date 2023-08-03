/*
 * YF
 * test.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_TEST_H
#define YF_TEST_H

#include <stddef.h>

#define YF_TEST_PRINT(fn_name, params, ret) \
    (*(ret) == '\0') ? (printf("\n%s(%s)\n", fn_name, params)) : \
                       (printf("\n%s(%s)\n -> %s\n", fn_name, params, ret))

#define YF_TEST_ALL "all"

/* Type defining test(s) to execute. */
typedef struct yf_test {
    char name[64];
    const char *const *ids;
    int (*const *fns)(void);
    size_t n;
} yf_test_t;

extern const yf_test_t yf_g_test;

#endif /* YF_TEST_H */
