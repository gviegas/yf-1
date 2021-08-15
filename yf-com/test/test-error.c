/*
 * YF
 * test-error.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "yf-error.h"

/* Tests error. */
int yf_test_error(void)
{
    int err;
    const size_t n = 128;
    char info[n];
    info[n-1] = info[0] = '\0';

    puts("(seterr YF_ERR_NOTFND,\"test\")\n");
    yf_seterr(YF_ERR_NOTFND, "test");
    puts("(geterr)");
    err = yf_geterr();
    printf(" %d\n\n", err);
    if (err != YF_ERR_NOTFND)
        return -1;

    printf("(geterrinfo info,%zu)\n", n);
    if (yf_geterrinfo(info, n) != info)
        return -1;
    printf(" %s\n\n", info);

    puts("(seterr YF_ERR_INVARG,NULL)\n");
    yf_seterr(YF_ERR_INVARG, NULL);
    if (err == yf_geterr())
        return -1;
    if (yf_geterr() != YF_ERR_INVARG)
        return -1;

    printf("(geterrinfo info,%zu)\n", n);
    if (yf_geterrinfo(info, n) != info)
        return -1;
    printf(" %s\n\n", info);

    puts("(seterr YF_ERR_OTHER,\"TEST\")\n");
    yf_seterr(YF_ERR_OTHER, "TEST");
    puts("(geterr)");
    err = yf_geterr();
    printf(" %d\n\n", err);
    if (err != YF_ERR_OTHER)
        return -1;

    printf("(geterrinfo info,%zu)\n", n);
    if (yf_geterrinfo(info, n) != info)
        return -1;
    printf(" %s\n", info);

    return 0;
}
