/*
 * YF
 * clock.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_CLOCK_H
#define YF_CLOCK_H

/* Gets the elapsed time, in seconds, relative to some unspecified point
   in the past. */
double yf_gettime(void);

/* Suspends execution for a given interval, in seconds. */
void yf_sleep(double seconds);

#endif /* YF_CLOCK_H */
