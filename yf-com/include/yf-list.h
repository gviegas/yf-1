/*
 * YF
 * yf-list.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_LIST_H
#define YF_YF_LIST_H

#include "yf-defs.h"
#include "yf-cmpfn.h"
#include "yf-iter.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a linked list.
 */
typedef struct yf_list yf_list_t;

/**
 * Initializes a new linked list.
 *
 * @param cmp: The comparison function to use. Can be 'NULL'.
 * @return: On success, returns a new list. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
yf_list_t *yf_list_init(yf_cmpfn_t cmp);

/**
 * Inserts a given value in a linked list.
 *
 * The value is inserted at the beginning of the list.
 *
 * Non-nil iterators that refer to 'list' become invalid.
 *
 * @param list: The list.
 * @param val: The value to insert.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_list_insert(yf_list_t *list, const void *val);

/**
 * Inserts a given value at a specific position of a linked list.
 *
 * The value is inserted after the iterator's current value.
 *
 * When called with an invalid iterator, this function is equivalent to
 * 'yf_list_insert()', i.e., 'val' is inserted at the beginning of the list.
 *
 * The iterator, if not 'NULL', is updated to point to the inserted value.
 * All other non-nil iterators that refer to 'list' become invalid.
 *
 * @param list: The list.
 * @param it: The iterator. Can be 'NULL'.
 * @param val: The value to insert.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_list_insertat(yf_list_t *list, yf_iter_t *it, const void *val);

/**
 * Removes a given value from a linked list.
 *
 * Non-nil iterators that refer to 'list' become invalid.
 *
 * @param list: The list.
 * @param val: The value to remove.
 * @return: If 'list' does not contain 'val', returns a non-zero value and
 *  sets the global error to 'YF_ERR_NOTFND'. Otherwise, zero is returned.
 */
int yf_list_remove(yf_list_t *list, const void *val);

/**
 * Removes the value from a specific position of a linked list.
 *
 * When called with an invalid iterator, this function removes the value
 * found at the beginning of the list.
 *
 * The iterator, if not 'NULL', is updated to point to the next value.
 * All other non-nil iterators that refer to 'list' become invalid.
 *
 * @param list: The list.
 * @param it: The iterator. Can be 'NULL'.
 * @return: If 'list' is empty, returns 'NULL'. Otherwise, the removed value
 *  is returned.
 */
void *yf_list_removeat(yf_list_t *list, yf_iter_t *it);

/**
 * Checks whether or not a linked list contains a given value.
 *
 * @param list: The list.
 * @param val: The value to check.
 * @return: If 'list' contains 'val', returns a non-zero value. Otherwise,
 *  zero is returned.
 */
int yf_list_contains(yf_list_t *list, const void *val);

/**
 * Gets the next value in a linked list.
 *
 * When the end of the list is reached, 'it' is set to the nil iterator.
 * One can check that there are no more elements to retrieve using the macro
 * 'YF_IT_ISNIL' - this is especially useful for lists that can contain the
 * 'NULL' value as a valid element (checking the returned value would suffice
 * otherwise).
 *
 * @param list: The list.
 * @param it: The iterator. Can be 'NULL'.
 * @return: If 'list' is empty, returns 'NULL'. If not, and 'it' is either
 *  'NULL' or the nil iterator, returns the first value from 'list'.
 *  Otherwise, the next value is returned.
 */
void *yf_list_next(yf_list_t *list, yf_iter_t *it);

/**
 * Executes a given function for each value in a linked list.
 *
 * This function completes when the end of the list is reached or when the
 * provided callback returns a non-zero value.
 *
 * The list must not be altered until 'yf_list_each()' completes.
 *
 * @param list: The list.
 * @param callb: The callback to execute for each value.
 * @param arg: The generic argument to pass on 'callb' calls. Can be 'NULL'.
 */
void yf_list_each(yf_list_t *list,
                  int (*callb)(void *val, void *arg), void *arg);

/**
 * Gets the number of elements stored in a linked list.
 *
 * @param list: The list.
 * @return: The length of the list.
 */
size_t yf_list_getlen(yf_list_t *list);

/**
 * Removes all elements from a linked list.
 *
 * Non-nil iterators that refer to 'list' become invalid.
 *
 * @param list: The list.
 */
void yf_list_clear(yf_list_t *list);

/**
 * Deinitializes a linked list.
 *
 * @param list: The list to deinitialize. Can be 'NULL'.
 */
void yf_list_deinit(yf_list_t *list);

YF_DECLS_END

#endif /* YF_YF_LIST_H */
