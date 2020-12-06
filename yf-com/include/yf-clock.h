/*
 * YF
 * yf-clock.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_CLOCK_H
#define YF_YF_CLOCK_H

#include "yf-defs.h"

YF_DECLS_BEGIN

/* Gets the elapsed time, in seconds, relative to some unspecified point
   in the past. */
double yf_gettime(void);

/* Suspends execution for a given interval, in seconds. */
void yf_sleep(double seconds);

YF_DECLS_END

#endif /* YF_YF_CLOCK_H */
