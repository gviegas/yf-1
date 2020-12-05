/*
 * YF
 * list.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_LIST_H
#define YF_LIST_H

#include "cmpfn.h"
#include "iter.h"

/* Opaque type defining a linked list. */
typedef struct YF_list_o *YF_list;

/* Initializes a new linked list. */
YF_list yf_list_init(YF_cmpfn cmp);

/* Inserts a given value in a linked list. */
int yf_list_insert(YF_list list, const void *val);

/* Removes a given value from a linked list. */
int yf_list_remove(YF_list list, const void *val);

/* Checks whether or not a linked list contains a given value. */
int yf_list_contains(YF_list list, const void *val);

/* Retrieves the next value for a given linked list iterator. */
void *yf_list_next(YF_list list, YF_iter *it);

/* Executes a given function for each value in a linked list. */
void yf_list_each(YF_list list, int (*fn)(void *val, void *arg), void *arg);

/* Gets the number of elements stored in a linked list. */
size_t yf_list_getlen(YF_list list);

/* Removes all elements from a linked list. */
void yf_list_clear(YF_list list);

/* Deinitializes a linked list. */
void yf_list_deinit(YF_list list);

#endif /* YF_LIST_H */
