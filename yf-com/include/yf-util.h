/*
 * YF
 * yf-util.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_UTIL_H
#define YF_YF_UTIL_H

/**
 * Computes the minimum of two values.
 */
#define YF_MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * Computes the maximum of two values.
 */
#define YF_MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * Clamps 'x' to the ['min','max'] range.
 */
#define YF_CLAMP(x, min, max) \
    ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#endif /* YF_YF_UTIL_H */
