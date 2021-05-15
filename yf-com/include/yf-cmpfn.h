/*
 * YF
 * yf-cmpfn.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_CMPFN_H
#define YF_YF_CMPFN_H

#include "yf-defs.h"

YF_DECLS_BEGIN

/**
 * Type defining a generic comparison function.
 */
typedef int (*YF_cmpfn)(const void *, const void *);

/**
 * Compares the values of two pointers.
 *
 * This is the default comparison function, which compares the pointers
 * themselves.
 *
 * @param ptr1: The first pointer.
 * @param ptr2: The second pointer.
 * @return: The result of subtracting the second pointer from the first.
 */
int yf_cmp(const void *ptr1, const void *ptr2);

YF_DECLS_END

#endif /* YF_YF_CMPFN_H */
