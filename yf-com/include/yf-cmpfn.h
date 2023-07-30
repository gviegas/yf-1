/*
 * YF
 * yf-cmpfn.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_CMPFN_H
#define YF_YF_CMPFN_H

#include "yf-defs.h"

YF_DECLS_BEGIN

/**
 * Type defining a generic comparison function.
 */
typedef int (*yf_cmpfn_t)(const void *, const void *);

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

/**
 * Compares two strings.
 *
 * @param str1: The fist string.
 * @param str2: The second string.
 * @return: 'strcmp(str1, str2)'.
 */
int yf_cmpstr(const void *str1, const void *str2);

YF_DECLS_END

#endif /* YF_YF_CMPFN_H */
