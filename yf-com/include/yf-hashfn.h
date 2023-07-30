/*
 * YF
 * yf-hashfn.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_HASHFN_H
#define YF_YF_HASHFN_H

#include <stddef.h>

#include "yf-defs.h"

YF_DECLS_BEGIN

/**
 * Type defining a generic hashing function.
 */
typedef size_t (*yf_hashfn_t)(const void *);

/**
 * Computes the hash value of a pointer.
 *
 * This is the default hashing function, which uses the pointer itself to
 * produce a hash value.
 *
 * @param ptr: The pointer.
 * @return: The hash value.
 */
size_t yf_hash(const void *ptr);

/**
 * Computes the hash value of a string.
 *
 * @param str: The null-terminated byte string.
 * @return: The hash value.
 */
size_t yf_hashstr(const void *str);

/**
 * Computes the hash value of a varying number of byte arrays.
 *
 * @param buf: The first buffer.
 * @param len: The length of the first buffer.
 * @param ...: Variable number of buffer/length pairs. A 'NULL' buffer signals
 *  the end of the argument list.
 * @return: The combined hash value.
 */
size_t yf_hashv(const void *buf, size_t len, ...);

YF_DECLS_END

#endif /* YF_YF_HASHFN_H */
