/*
 * YF
 * node.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_NODE_H
#define YF_NODE_H

#include "yf-node.h"

/* Sets the object which a given node is to represent. */
void yf_node_setobj(YF_node node, int nodeobj, void *obj,
                    void (*deinit)(void *obj));

/* Gets the world transform of a node. */
YF_mat4 *yf_node_getwldxform(YF_node node);

/* Gets the inverse world transform of a node. */
YF_mat4 *yf_node_getwldinv(YF_node node);

/* Gets the normal matrix of a node. */
YF_mat4 *yf_node_getwldnorm(YF_node node);

#endif /* YF_NODE_H */
