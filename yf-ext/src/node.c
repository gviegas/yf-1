/*
 * YF
 * node.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "node.h"
#include "error.h"

struct YF_node_o {
  YF_node parent;
  YF_node prev_sibl;
  YF_node next_sibl;
  YF_node child;
  size_t n;
  int nodeobj;
  void *obj;
};

YF_node yf_node_init(void) {
  YF_node node = malloc(sizeof(struct YF_node_o));
  if (node == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  node->parent = NULL;
  node->prev_sibl = NULL;
  node->next_sibl = NULL;
  node->child = NULL;
  node->n = 1;
  node->nodeobj = YF_NODEOBJ_NONE;
  node->obj = NULL;
  return node;
}

void yf_node_insert(YF_node node, YF_node child) {
  assert(node != NULL);
  assert(child != NULL);
  assert(node != child);

  if (child->parent != NULL)
    yf_node_drop(child);

  child->parent = node;
  child->prev_sibl = NULL;
  child->next_sibl = node->child;
  if (child->next_sibl != NULL)
    child->next_sibl->prev_sibl = child;
  node->child = child;

  YF_node aux = node;
  do
    aux->n += child->n;
  while ((aux = aux->parent) != NULL);
}

void yf_node_drop(YF_node node) {
  assert(node != NULL);
  if (node->parent == NULL)
    return;

  if (node->next_sibl != NULL)
    node->next_sibl->prev_sibl = node->prev_sibl;
  if (node->prev_sibl != NULL)
    node->prev_sibl->next_sibl = node->next_sibl;
  else
    node->parent->child = node->next_sibl;

  YF_node aux = node->parent;
  do
    aux->n -= node->n;
  while ((aux = aux->parent) != NULL);

  node->parent = NULL;
  node->prev_sibl = NULL;
  node->next_sibl = NULL;
}

void yf_node_prune(YF_node node) {
  assert(node != NULL);
  if (node->child == NULL)
    return;

  YF_node curr = node->child;
  YF_node next;
  size_t n = 0;
  do {
    next = curr->next_sibl;
    curr->parent = NULL;
    curr->prev_sibl = NULL;
    curr->next_sibl = NULL;
    n += curr->n;
    curr = next;
  } while (next != NULL);
  node->child = NULL;

  YF_node aux = node;
  do
    aux->n -= n;
  while ((aux = aux->parent) != NULL);
}

int yf_node_traverse(
  YF_node node,
  int (*fn)(YF_node descendant, void *arg),
  void *arg)
{
  assert(node != NULL);
  assert(fn != NULL);
  if (node->child == NULL)
    return 0;

  YF_node *queue = malloc(node->n * sizeof(YF_node));
  if (queue == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  queue[0] = node;
  YF_node next = NULL;
  size_t last_i = 0;
  size_t curr_i = 0;
  do {
    next = queue[curr_i]->child;
    while (next != NULL) {
      if (fn(next, arg) != 0) {
        free(queue);
        return 0;
      }
      if (next->child != NULL)
        queue[++last_i] = next;
      next = next->next_sibl;
    }
  } while (++curr_i <= last_i);
  free(queue);
  return 0;
}

int yf_node_descends(YF_node node, YF_node ancestor) {
  assert(node != NULL);
  assert(ancestor != NULL);
  assert(node != ancestor);

  YF_node anc = node->parent;
  while (anc != NULL) {
    if (anc != ancestor)
      anc = anc->parent;
    else
      return 1;
  }
  return 0;
}

int yf_node_isleaf(YF_node node) {
  assert(node != NULL);
  return node->child == NULL;
}

size_t yf_node_getlen(YF_node node) {
  assert(node != NULL);
  return node->n;
}

int yf_node_getobj(YF_node node, void **obj) {
  assert(node != NULL);
  if (obj != NULL)
    *obj = node->obj;
  return node->nodeobj;
}

void yf_node_deinit(YF_node node) {
  if (node != NULL) {
    yf_node_drop(node);
    yf_node_prune(node);
    free(node);
  }
}

void yf_node_setobj(YF_node node, int nodeobj, void *obj) {
  assert(node != NULL);
  /* TODO: Assert that `nodeobj` is valid. */
  node->nodeobj = nodeobj;
  node->obj = obj;
}
