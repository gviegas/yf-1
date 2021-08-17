/*
 * YF
 * test-error.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "test.h"
#include "yf-error.h"

/* Tests error. */
int yf_test_error(void)
{
    int err;
    const size_t n = 128;
    char info[n];
    info[n-1] = info[0] = '\0';

    YF_TEST_PRINT("seterr", "ERR_NOTFND, \"test\"", "");
    yf_seterr(YF_ERR_NOTFND, "test");
    YF_TEST_PRINT("geterr", "", "");
    err = yf_geterr();
    if (err != YF_ERR_NOTFND)
        return -1;

    YF_TEST_PRINT("geterrinfo", "info, n", "");
    if (yf_geterrinfo(info, n) != info)
        return -1;

    YF_TEST_PRINT("seterr", "ERR_INVARG, NULL", "");
    yf_seterr(YF_ERR_INVARG, NULL);
    if (err == yf_geterr())
        return -1;
    if (yf_geterr() != YF_ERR_INVARG)
        return -1;

    YF_TEST_PRINT("geterrinfo", "info, n", "");
    if (yf_geterrinfo(info, n) != info)
        return -1;

    YF_TEST_PRINT("seterr", "ERR_OTHER, \"TEST\"", "");
    yf_seterr(YF_ERR_OTHER, "TEST");
    YF_TEST_PRINT("geterr", "", "");
    err = yf_geterr();
    if (err != YF_ERR_OTHER)
        return -1;

    YF_TEST_PRINT("geterrinfo", "info, n", "");
    if (yf_geterrinfo(info, n) != info)
        return -1;

    return 0;
}
