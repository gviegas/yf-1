/*
 * YF
 * yf-node.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_NODE_H
#define YF_YF_NODE_H

#include <stddef.h>

#include "yf-common.h"

YF_DECLS_BEGIN

/* Opaque type defining a scene node. */
typedef struct YF_node_o *YF_node;

/* Initializes a new node. */
YF_node yf_node_init(void);

/* Inserts a node as child of another. */
void yf_node_insert(YF_node node, YF_node child);

/* Removes a node from its parent. */
void yf_node_drop(YF_node node);

/* Removes all child nodes from a given node. */
void yf_node_prune(YF_node node);

/* Executes a given function for every descendant of a node. */
int yf_node_traverse(
  YF_node node,
  int (*fn)(YF_node descendant, void *arg),
  void *arg);

/* Checks whether or not a given node descends from another. */
int yf_node_descends(YF_node node, YF_node ancestor);

/* Checks whether or not a given node has no descendants. */
int yf_node_isleaf(YF_node node);

/* Gets the length of a node's subgraph. */
size_t yf_node_getlen(YF_node node);

/* Gets the object that a given node represents. */
int yf_node_getobj(YF_node node, void **obj);

/* Deinitializes a node. */
void yf_node_deinit(YF_node node);

/* Types of objects that a node can represent. */
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
