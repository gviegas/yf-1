/*
 * YF
 * yf-clock.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_CLOCK_H
#define YF_YF_CLOCK_H

#include "yf-defs.h"

YF_DECLS_BEGIN

/**
 * Gets the elapsed time relative to some unspecified point in the past.
 *
 * Values returned by subsequent calls to 'yf_gettime()' can be used to
 * calculate elapsed time. The precision is implementation-dependent.
 *
 * @return: The elapsed time, in seconds.
 */
double yf_gettime(void);

/**
 * Suspends execution of the calling thread.
 *
 * This function is signal-aware and will attempt to resume sleeping when
 * interrupted.
 *
 * @param seconds: The interval to sleep for, in seconds.
 */
void yf_sleep(double seconds);

YF_DECLS_END

#endif /* YF_YF_CLOCK_H */
