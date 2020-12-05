/*
 * YF
 * yf-hashfn.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_HASHFN_H
#define YF_YF_HASHFN_H

#include <stddef.h>

/* Type defining a generic hashing function. */
typedef size_t (*YF_hashfn)(const void *);

/* Computes a hash value from a pointer.
   This is the default hashing function, performed on the pointer. */
size_t yf_hash(const void *ptr);

#endif /* YF_YF_HASHFN_H */
