/*
 * YF
 * yf-collection.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_COLLECTION_H
#define YF_YF_COLLECTION_H

#include "yf/com/yf-defs.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a collection of objects.
 */
typedef struct YF_collection_o *YF_collection;

/**
 * Item types indicating what can be stored in a collection.
 */
#define YF_CITEM_SCENE    0
#define YF_CITEM_NODE     1
#define YF_CITEM_MESH     2
#define YF_CITEM_SKIN     3
#define YF_CITEM_MATERIAL 4
#define YF_CITEM_TEXTURE  5
#define YF_CITEM_FONT     6

#define YF_CITEM_N 7

/**
 * Initializes a new collection.
 *
 * @param pathname: The pathname of a file containing assets to load
 *  during initialization. Can be 'NULL'.
 * @return: On success, returns a new collection. Otherwise, 'NULL' is
 *  returned and the global error is set to indicate the cause.
 */
YF_collection yf_collection_init(const char *pathname);

/**
 * Gets an item stored in a collection.
 *
 * @param coll: The collection.
 * @param citem: The 'YF_CITEM' value indicating the item type.
 * @param name: The name of the item.
 * @return: On success, returns the item. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
void *yf_collection_getitem(YF_collection coll, int citem, const char *name);

/**
 * Manages an item using a collection.
 *
 * @param coll: The collection.
 * @param citem: The 'YF_CITEM' value indicating the item type.
 * @param name: The name to use as identifier for the item. Can be 'NULL'.
 * @param item: The item to manage.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_collection_manage(YF_collection coll, int citem, const char *name,
                         void *item);

/**
 * Releases an item from a collection.
 *
 * @param coll: The collection.
 * @param citem: The 'YF_CITEM' value indicating the item type.
 * @param name: The name of the item to release.
 * @return: On success, returns the item. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
void *yf_collection_release(YF_collection coll, int citem, const char *name);

/**
 * Checks whether or not a collection contains a given item.
 *
 * @param coll: The collection.
 * @param citem: The 'YF_CITEM' value indicating the item type.
 * @param name: The name of the item to check.
 * @return: If 'coll' contains an item of type 'citem' named 'name', returns
 *  a non-zero value. Otherwise, zero is returned.
 */
int yf_collection_contains(YF_collection coll, int citem, const char *name);

/**
 * Executes a given function for each entry of a given item type.
 *
 * @param coll: The collection.
 * @param citem: The 'YF_CITEM' value indicating the item type.
 * @param callb: The callback to execute for each entry.
 * @param arg: The generic argument to pass on 'callb' calls. Can be 'NULL'.
 */
void yf_collection_each(YF_collection coll, int citem,
                        int (*callb)(void *name, void *item, void *arg),
                        void *arg);

/**
 * Deinitializes a collection.
 *
 * This function implicitly calls 'deinit()' for every object managed by
 * the collection.
 *
 * @param coll: The collection to deinitialize. Can be 'NULL'.
 */
void yf_collection_deinit(YF_collection coll);

YF_DECLS_END

#endif /* YF_YF_COLLECTION_H */
