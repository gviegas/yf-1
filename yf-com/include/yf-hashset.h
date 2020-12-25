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

/**
 * Opaque type defining a hashset.
 */
typedef struct YF_hashset_o *YF_hashset;

/**
 * Initializes a new hashset.
 *
 * @param hash: The hashing function to use.
 * @param cmp: The comparison function to use.
 * @return: On success, returns a new hashset. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_hashset yf_hashset_init(YF_hashfn hash, YF_cmpfn cmp);

/**
 * Inserts a given value in a hashset.
 *
 * Non-nil iterators that refer to 'set' become invalid.
 *
 * @param set: The hashset.
 * @param val: The value to insert.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_hashset_insert(YF_hashset set, const void *val);

/**
 * Removes a given value from a hashset.
 *
 * Non-nil iterators that refer to 'set' become invalid.
 *
 * @param set: The hashset.
 * @param val: The value to remove.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_hashset_remove(YF_hashset set, const void *val);

/**
 * Checks whether or not a hashset contains a given value.
 *
 * @param set: The hashset.
 * @param val: The value to check.
 * @return: If 'set' contains 'val', returns a non-zero value. Otherwise,
 *  zero is returned.
 */
int yf_hashset_contains(YF_hashset set, const void *val);

/**
 * Searches for a given value in a hashset.
 *
 * The purpose of this function is to allow a hashset to be used as a
 * dictionary. This is possible because any two values that produce the
 * same hash and compare equal are considered to be the same element.
 *
 * @param set: The hashset.
 * @param val: The value to search for.
 * @return: If 'val' is found, returns the value stored in the hashset.
 *  Otherwise, 'NULL' is returned.
 */
void *yf_hashset_search(YF_hashset set, const void *val);

/**
 * Removes and returns the current value for a given hashset iterator.
 *
 * This function extracts random elements when 'it' is either the nil
 * iterator or 'NULL'.
 *
 * When provided, 'it' is set to the nil iterator at the time this function
 * returns. Thus, for hashsets that might contain 'NULL' as a valid element,
 * one would need to call 'yf_hashset_getlen' to check whether or not the
 * hashset is empty.
 *
 * Non-nil iterators that refer to 'set' become invalid.
 *
 * @param set: The hashset.
 * @param it: The iterator. Can be 'NULL'.
 * @return: If 'set' is empty, returns 'NULL'. If not, and 'it' is either
 *  'NULL' or the nil iterator, returns an unspecified value from 'set'.
 *  Otherwise, the current value is returned.
 */
void *yf_hashset_extract(YF_hashset set, YF_iter *it);

/**
 * Retrieves the next value for a given hashset iterator.
 *
 * When the end of the hashset is reached, 'it' is set to the nil iterator.
 * One can check that there are no more elements to retrieve using the macro
 * 'YF_IT_ISNIL' - this is especially useful for sets that can contain the
 * 'NULL' value as a valid element (checking the return value would suffice
 * otherwise).
 *
 * @param set: The hashset.
 * @param it: The iterator. Can be 'NULL'.
 * @return: If 'set' is empty, returns 'NULL'. If not, and 'it' is either
 *  'NULL' or the nil iterator, returns the first value from 'set'.
 *  Otherwise, the next value is returned.
 */
void *yf_hashset_next(YF_hashset set, YF_iter *it);

/**
 * Executes a given function for each value in a hashset.
 *
 * This function completes when the end of the hashset is reached or when
 * the provided callback returns a non-zero value.
 *
 * The hashset must not be altered until 'yf_hashset_each' completes.
 *
 * @param set: The hashset.
 * @param callb: The callback to execute for each value.
 * @param arg: The generic argument to pass on 'callb' calls. Can be 'NULL'.
 */
void yf_hashset_each(YF_hashset set, int (*callb)(void *val, void *arg),
    void *arg);

/**
 * Gets the number of elements stored in a hashset.
 *
 * @param set: The hashset.
 * @return: The length of the hashset.
 */
size_t yf_hashset_getlen(YF_hashset set);

/**
 * Removes all elements from a hashset.
 *
 * Non-nil iterators that refer to 'set' become invalid.
 *
 * @param set: The hashset.
 */
void yf_hashset_clear(YF_hashset set);

/**
 * Deinitializes a hashset.
 *
 * @param set: The hashset to deinitialize. Can be 'NULL'.
 */
void yf_hashset_deinit(YF_hashset set);

YF_DECLS_END

#endif /* YF_YF_HASHSET_H */
