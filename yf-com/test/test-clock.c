/*
 * YF
 * test-clock.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "test.h"
#include "yf-clock.h"

/* Tests clock. */
int yf_test_clock(void)
{
    double t1, t2;
    const double ts[] = {1.0, 1.5, 0.1, 0.01, 3.125};
    char s[256] = {0};

    t1 = yf_gettime();
    snprintf(s, sizeof s, "%.10f", t1);
    YF_TEST_PRINT("gettime", "", s);

    t2 = yf_gettime();
    snprintf(s, sizeof s, "%.10f", t2);
    YF_TEST_PRINT("gettime", "", s);

    printf("\n- elapsed time: %.10fs -\n\n", t2 - t1);

    for (size_t i = 0; i < (sizeof ts / sizeof *ts); i++) {
        snprintf(s, sizeof s, "%.3f", ts[i]);
        YF_TEST_PRINT("sleep", s, "");
        t1 = yf_gettime();
        yf_sleep(ts[i]);
        printf("\n- elapsed time: %.10fs -\n\n", yf_gettime() - t1);
    }

    return 0;
}
