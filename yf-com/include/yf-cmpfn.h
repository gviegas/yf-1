/*
 * YF
 * yf-cmpfn.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_CMPFN_H
#define YF_YF_CMPFN_H

/* Type defining a generic comparison function. */
typedef int (*YF_cmpfn)(const void *, const void *);

/* Compares the values of two pointers.
   This is the default comparison function, performed on the pointers. */
int yf_cmp(const void *ptr1, const void *ptr2);

#endif /* YF_YF_CMPFN_H */
