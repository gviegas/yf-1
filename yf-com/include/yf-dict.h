/*
 * YF
 * yf-dict.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_DICT_H
#define YF_YF_DICT_H

#include "yf-defs.h"
#include "yf-hashfn.h"
#include "yf-cmpfn.h"
#include "yf-iter.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a dictionary.
 */
typedef struct YF_dict_o *YF_dict;

/**
 * Initializes a new dictionary.
 *
 * @param hash: The hashing function to use. Can be 'NULL'.
 * @param cmp: The comparison function to use. Can be 'NULL'.
 * @return: On success, returns a new dictionary. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_dict yf_dict_init(YF_hashfn hash, YF_cmpfn cmp);

/**
 * Inserts a key/value pair in a dictionary.
 *
 * @param dict: The dictionary.
 * @param key: The key.
 * @param val: The value.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_dict_insert(YF_dict dict, const void *key, const void *val);

/**
 * Removes a key/value pair from a dictionary.
 *
 * @param dict: The dictionary.
 * @param key: The key.
 * @return: If 'dict' does not contain 'key', returns 'NULL' and sets the
 *  global error to 'YF_ERR_NOTFND'. Otherwise, returns the removed value.
 */
void *yf_dict_remove(YF_dict dict, const void *key);

/**
 * Removes a key/value pair from a dictionary.
 *
 * @param dict: The dictionary.
 * @param key: The key location, whose value will be set to the stored key.
 * @return: If 'dict' does not contain '*key', returns 'NULL' and sets the
 *  global error to 'YF_ERR_NOTFND'. Otherwise, returns the removed value.
 */
void *yf_dict_delete(YF_dict dict, void **key);

/**
 * Replaces the value stored under a given key of a dictionary.
 *
 * @param dict: The dictionary.
 * @param key: The key.
 * @param val: The new value.
 * @return: If 'dict' does not contain 'key', returns 'NULL' and sets the
 *  global error to 'YF_ERR_NOTFND'. Otherwise, returns the old value.
 */
void *yf_dict_replace(YF_dict dict, const void *key, const void *val);

/**
 * Searches for a key in a dictionary.
 *
 * @param dict: The dictionary.
 * @param key: The key.
 * @return: If 'dict' does not contain 'key', returns 'NULL' and sets the
 *  global error to 'YF_ERR_NOTFND'. Otherwise, returns the stored value.
 */
void *yf_dict_search(YF_dict dict, const void *key);

/**
 * Searches for a key in a dictionary.
 *
 * @param dict: The dictionary.
 * @param key: The key location, whose value will be set to the stored key.
 * @return: If 'dict' does not contain '*key', returns 'NULL' and sets the
 *  global error to 'YF_ERR_NOTFND'. Otherwise, returns the stored value.
 */
void *yf_dict_lookup(YF_dict dict, void **key);

/**
 * Checks whether or not a dictionary contains a given key.
 *
 * @param dict: The dictionary.
 * @param key: The key.
 * @return: If 'dict' contains 'key', returns a non-zero value. Otherwise,
 *  zero is returned.
 */
int yf_dict_contains(YF_dict dict, const void *key);

/**
 * Gets the next key/value pair in a dictionary.
 *
 * @param dict: The dictionary.
 * @param it: The iterator. Can be 'NULL'.
 * @param key: The destination for the key. Can be 'NULL'.
 * @return: The next value, or 'NULL' if there are no more values to retrieve.
 */
void *yf_dict_next(YF_dict dict, YF_iter *it, void **key);

/**
 * Executes a given function for each key/value pair in a dictionary.
 *
 * @param dict: The dictionary.
 * @param callb: The callback to execute for each key/value pair.
 * @param arg: The generic argument to pass on 'callb' calls. Can be 'NULL'.
 */
void yf_dict_each(YF_dict dict, int (*callb)(void *key, void *val, void *arg),
                  void *arg);

/**
 * Gets the number of key/value pairs stored in a dictionary.
 *
 * @param dict: The dictionary.
 * @return: The length of the dictionary.
 */
size_t yf_dict_getlen(YF_dict dict);

/**
 * Removes all key/value pairs from a dictionary.
 *
 * @param dict: The dictionary.
 */
void yf_dict_clear(YF_dict dict);

/**
 * Deinitializes a dictionary.
 *
 * @param dict: The dictionary to deinitialize. Can be 'NULL'.
 */
void yf_dict_deinit(YF_dict dict);

YF_DECLS_END

#endif /* YF_YF_DICT_H */
