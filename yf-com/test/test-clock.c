/*
 * YF
 * test-clock.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "yf-clock.h"

/* Tests clock. */
int yf_test_clock(void)
{
    double t1, t2;
    const double ts[] = {1.0, 1.5, 0.1, 0.01, 3.125};

    puts("(gettime)");
    t1 = yf_gettime();
    printf(" %.10f\n\n", t1);

    puts("(gettime)");
    t2 = yf_gettime();
    printf(" %.10f\n\n", t2);

    printf("elapsed time: %.10fs\n\n", t2 - t1);

    for (size_t i = 0; i < (sizeof ts / sizeof *ts); i++) {
        printf("(sleep %.3f)\n\n", ts[i]);
        t1 = yf_gettime();
        yf_sleep(ts[i]);
        printf("elapsed time: %.10fs\n\n", yf_gettime() - t1);
    }

    return 0;
}
