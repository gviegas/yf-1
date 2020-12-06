/*
 * YF
 * yf-hashset.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_HASHSET_H
#define YF_YF_HASHSET_H

#include "yf-defs.h"
#include "yf-hashfn.h"
#include "yf-cmpfn.h"
#include "yf-iter.h"

YF_DECLS_BEGIN

/* Opaque type defining a hashset. */
typedef struct YF_hashset_o *YF_hashset;

/* Initializes a new hashset. */
YF_hashset yf_hashset_init(YF_hashfn hash, YF_cmpfn cmp);

/* Inserts a given value in a hashset. */
int yf_hashset_insert(YF_hashset set, const void *val);

/* Removes a given value from a hashset. */
int yf_hashset_remove(YF_hashset set, const void *val);

/* Checks whether or not a hashset contains a given value. */
int yf_hashset_contains(YF_hashset set, const void *val);

/* Searches for a given value in a hashset. */
void *yf_hashset_search(YF_hashset set, const void *val);

/* Removes and returns the current value for a given hashset iterator. */
void *yf_hashset_extract(YF_hashset set, YF_iter *it);

/* Retrieves the next value for a given hashset iterator. */
void *yf_hashset_next(YF_hashset set, YF_iter *it);

/* Executes a given function for each value in a hashset. */
void yf_hashset_each(
  YF_hashset set,
  int (*fn)(void *val, void *arg),
  void *arg);

/* Gets the number of elements stored in a hashset. */
size_t yf_hashset_getlen(YF_hashset set);

/* Removes all elements from a hashset. */
void yf_hashset_clear(YF_hashset set);

/* Deinitializes a hashset. */
void yf_hashset_deinit(YF_hashset set);

YF_DECLS_END

#endif /* YF_YF_HASHSET_H */
