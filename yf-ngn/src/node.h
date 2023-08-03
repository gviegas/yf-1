/*
 * YF
 * node.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_NODE_H
#define YF_NODE_H

#include "yf-node.h"

/* Sets the object which a given node is to represent. */
void yf_node_setobj(yf_node_t *node, int nodeobj, void *obj,
                    void (*deinit)(void *obj));

/* Gets the world transform of a node. */
yf_mat4_t *yf_node_getwldxform(yf_node_t *node);

/* Gets the inverse world transform of a node. */
yf_mat4_t *yf_node_getwldinv(yf_node_t *node);

/* Gets the normal matrix of a node. */
yf_mat4_t *yf_node_getwldnorm(yf_node_t *node);

#endif /* YF_NODE_H */
