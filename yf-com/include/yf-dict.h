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
 * @return: The removed value, or 'NULL' if not found.
 */
void *yf_dict_remove(YF_dict dict, const void *key);

/**
 * Searches for a key in a dictionary.
 *
 * @param dict: The dictionary.
 * @param key: The key.
 * @return: The value stored under the given key, or 'NULL' if not found.
 */
void *yf_dict_search(YF_dict dict, const void *key);

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
 * Gets the number of key/value pairs stored in a dictionary.
 *
 * @param dict: The dictionary.
 * @return: The length of the dictionary.
 */
size_t yf_dict_getlen(YF_dict dict);

/**
 * Deinitializes a dictionary.
 *
 * @param dict: The dictionary to deinitialize. Can be 'NULL'.
 */
void yf_dict_deinit(YF_dict dict);

YF_DECLS_END

#endif /* YF_YF_DICT_H */
