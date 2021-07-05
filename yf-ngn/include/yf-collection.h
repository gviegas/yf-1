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
 * Opaque type defining a collection of resources.
 */
typedef struct YF_collection_o *YF_collection;

/**
 * Types of resources that a collection can store.
 */
#define YF_COLLRES_SCENE    0
#define YF_COLLRES_NODE     1
#define YF_COLLRES_MESH     2
#define YF_COLLRES_MATERIAL 3
#define YF_COLLRES_TEXTURE  4
#define YF_COLLRES_FONT     5

#define YF_COLLRES_N 6

/**
 * Initializes a new collection.
 *
 * @param pathname: The pathname of a file containing resources to load
 *  during initialization. Can be 'NULL'.
 * @return: On success, returns a new collection. Otherwise, 'NULL' is
 *  returned and the global error is set to indicate the cause.
 */
YF_collection yf_collection_init(const char *pathname);

/**
 * Gets a resource stored in a collection.
 *
 * @param coll: The collection.
 * @param collres: The 'YF_COLLRES' value indicating the type of the resource.
 * @param name: The name of the resource.
 * @return: On success, returns the resource. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
void *yf_collection_getres(YF_collection coll, int collres, const char *name);

/**
 * Manages a resource using a collection.
 *
 * @param coll: The collection.
 * @param collres: The 'YF_COLLRES' value indicating the type of the resource.
 * @param name: The name to use as resource identifier.
 * @param res: The resource to manage.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_collection_manage(YF_collection coll, int collres, const char *name,
                         void *res);

/**
 * Releases a resource from a collection.
 *
 * @param coll: The collection.
 * @param collres: The 'YF_COLLRES' value indicating the type of the resource.
 * @param name: The name of the resource to be released.
 * @return: On success, returns the resource. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
void *yf_collection_release(YF_collection coll, int collres, const char *name);

/**
 * Checks whether or not a collection contains a given resource.
 *
 * @param coll: The collection.
 * @param collres: The 'YF_COLLRES' value indicating the type of the resource.
 * @param name: The name of the resource.
 * @return: If 'coll' contains a resource of type 'collres' named 'name',
 *  returns a non-zero value. Otherwise, zero is returned.
 */
int yf_collection_contains(YF_collection coll, int collres, const char *name);

/**
 * Deinitializes a collection.
 *
 * This function implicitly calls 'deinit()' for every resource managed by
 * the collection.
 *
 * @param coll: The collection to deinitialize. Can be 'NULL'.
 */
void yf_collection_deinit(YF_collection coll);

YF_DECLS_END

#endif /* YF_YF_COLLECTION_H */
