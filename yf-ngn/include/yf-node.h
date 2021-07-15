/*
 * YF
 * yf-node.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_NODE_H
#define YF_YF_NODE_H

#include <stddef.h>

#include "yf/com/yf-defs.h"

#include "yf-matrix.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a scene node.
 */
typedef struct YF_node_o *YF_node;

/**
 * Initializes a new node.
 *
 * @return: On success, returns a new node. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_node yf_node_init(void);

/**
 * Inserts a node as child of another.
 *
 * @param node: The node.
 * @param child: The node to insert.
 */
void yf_node_insert(YF_node node, YF_node child);

/**
 * Removes a node from its parent.
 *
 * @param node: The node.
 */
void yf_node_drop(YF_node node);

/**
 * Removes all child nodes from a given node.
 *
 * @param node: The node.
 */
void yf_node_prune(YF_node node);

/**
 * Executes a given function for every descendant of a node.
 *
 * This function will visit every descendant of 'node' and execute the
 * provided callback. It stops when there is no more descendants of 'node'
 * to visit or when the callback returns a non-zero value.
 *
 * @param node: The node.
 * @param callb: The callback to execute for each descendant.
 * @param arg: The generic argument to pass on 'callb' calls. Can be 'NULL'.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_node_traverse(YF_node node, int (*callb)(YF_node descendant, void *arg),
                     void *arg);

/**
 * Checks whether or not a given node descends from another.
 *
 * @param node: The node to check.
 * @param ancestor: The ancestor node.
 * @return: If 'node' descends from 'ancestor', returns a non-zero value.
 *  Otherwise, zero is returned.
 */
int yf_node_descends(YF_node node, YF_node ancestor);

/**
 * Checks whether or not a given node has no descendants.
 *
 * @param node: The node to check.
 * @return: If 'node' is a leaf node, returns a non-zero value. Otherwise,
 *  zero is returned.
 */
int yf_node_isleaf(YF_node node);

/**
 * Checks whether or not a given node has no ancestors.
 *
 * @param node: The node to check.
 * @return: If 'node' is a root node, returns a non-zero value. Otherwise,
 *  zero is returned.
 */
int yf_node_isroot(YF_node node);

/**
 * Gets the immediate ancestor of a node.
 *
 * @param node: The node.
 * @return: The parent of 'node', or 'NULL' if 'node' is a root node.
 */
YF_node yf_node_getparent(YF_node node);

/**
 * Gets the length of a node's subgraph.
 *
 * @param node: The node.
 * @return: The length of the subgraph. This value will be at least one.
 */
size_t yf_node_getlen(YF_node node);

/**
 * Gets the transformation matrix of a node.
 *
 * @param node: The node.
 * @return: The node's transformation matrix.
 */
YF_mat4 *yf_node_getxform(YF_node node);

/**
 * Gets the name of a node.
 *
 * @param node: The node.
 * @param dst: The destination for the name. Can be 'NULL'.
 * @param n: The number of characters that 'dst' can contain. When 'dst' is
 *  not 'NULL', '*n' must be greater than zero. This location is updated to
 *  contain the name size, including the terminating null byte.
 * @return: If the length of the name exceeds 'n', returns 'NULL'.
 *  Otherwise, returns 'dst'.
 */
char *yf_node_getname(YF_node node, char *dst, size_t *n);

/**
 * Sets the name for a node.
 *
 * @param node: The node.
 * @param name: The name to set. Can be 'NULL'.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_node_setname(YF_node node, const char *name);

/**
 * Compares the name of a node with a given string.
 *
 * @param node: The node.
 * @param str: The string to compare.
 * @return: A value greater than, equal to, or less than zero if the node's
 *  name is, respectively, greater than, equal to, or less than 'str'.
 */
int yf_node_cmpname(YF_node node, const char *str);

/**
 * Gets the object that a given node represents.
 *
 * @param node: The node.
 * @param obj: The destination for the object. Can be 'NULL'.
 * @return: The 'YF_NODEOBJ' value indicating the object type.
 */
int yf_node_getobj(YF_node node, void **obj);

/**
 * Deinitializes a node.
 *
 * @param node: The node to deinitialize. Can be 'NULL'.
 */
void yf_node_deinit(YF_node node);

/**
 * Types of objects that a node can represent.
 */
#define YF_NODEOBJ_NONE     0
#define YF_NODEOBJ_MODEL    1
#define YF_NODEOBJ_TERRAIN  2
#define YF_NODEOBJ_PARTICLE 3
#define YF_NODEOBJ_QUAD     4
#define YF_NODEOBJ_LABEL    5
#define YF_NODEOBJ_LIGHT    6
#define YF_NODEOBJ_EFFECT   7

YF_DECLS_END

#endif /* YF_YF_NODE_H */
