/*
 * YF
 * clock.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <time.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

#include "yf-clock.h"

double yf_gettime(void)
{
    double tm = 0.0;

#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 199309L)
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    assert(ts.tv_sec + 1 < DBL_MAX);

    tm = (double)ts.tv_sec + (double)ts.tv_nsec * 1.0e-9;
#else
    /* TODO: Other platforms. */
# error "Invalid platform"
#endif

    return tm;
}

void yf_sleep(double seconds)
{
    assert(seconds >= 0.0);

#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 199309L)
    struct timespec ts, ts_rem;
    ts.tv_sec = seconds;
    ts.tv_nsec = (seconds - floor(seconds)) * 1.0e9;
    int r;

    while ((r = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts_rem)) != 0) {
        assert(r == EINTR);
        ts = ts_rem;
    }
#else
    /* TODO: Other platforms. */
# error "Invalid platform"
#endif
}
