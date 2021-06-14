/*
 * YF
 * node.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "node.h"

struct YF_node_o {
    YF_node parent;
    YF_node prev_sibl;
    YF_node next_sibl;
    YF_node child;
    size_t n;

    YF_mat4 xform;
    char *name;

    int nodeobj;
    void *obj;
    void (*deinit)(void *);

    YF_mat4 wld_xform;
    YF_mat4 wld_inv;
};

YF_node yf_node_init(void)
{
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
    yf_mat4_iden(node->xform);
    node->name = NULL;
    node->nodeobj = YF_NODEOBJ_NONE;
    node->obj = NULL;
    node->deinit = NULL;
    yf_mat4_iden(node->wld_xform);
    yf_mat4_iden(node->wld_inv);

    return node;
}

void yf_node_insert(YF_node node, YF_node child)
{
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

void yf_node_drop(YF_node node)
{
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

void yf_node_prune(YF_node node)
{
    assert(node != NULL);

    if (node->child == NULL)
        return;

    YF_node cur = node->child;
    YF_node next;
    size_t n = 0;
    do {
        next = cur->next_sibl;
        cur->parent = NULL;
        cur->prev_sibl = NULL;
        cur->next_sibl = NULL;
        n += cur->n;
        cur = next;
    } while (next != NULL);
    node->child = NULL;

    YF_node aux = node;
    do
        aux->n -= n;
    while ((aux = aux->parent) != NULL);
}

int yf_node_traverse(YF_node node, int (*fn)(YF_node descendant, void *arg),
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
    size_t cur_i = 0;
    do {
        next = queue[cur_i]->child;
        while (next != NULL) {
            if (fn(next, arg) != 0) {
                free(queue);
                return 0;
            }
            if (next->child != NULL)
                queue[++last_i] = next;
            next = next->next_sibl;
        }
    } while (++cur_i <= last_i);
    free(queue);
    return 0;
}

int yf_node_descends(YF_node node, YF_node ancestor)
{
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

int yf_node_isleaf(YF_node node)
{
    assert(node != NULL);
    return node->child == NULL;
}

int yf_node_isroot(YF_node node)
{
    assert(node != NULL);
    return node->parent == NULL;
}

YF_node yf_node_getparent(YF_node node)
{
    assert(node != NULL);
    return node->parent;
}

size_t yf_node_getlen(YF_node node)
{
    assert(node != NULL);
    return node->n;
}

YF_mat4 *yf_node_getxform(YF_node node)
{
    assert(node != NULL);
    return &node->xform;
}

char *yf_node_getname(YF_node node, char *dst, size_t n)
{
    assert(node != NULL);
    assert(dst != NULL);
    assert(n > 0);

    if (node->name == NULL) {
        dst[0] = '\0';
        return dst;
    }

    if (strlen(node->name) < n)
        return strcpy(dst, node->name);
    return NULL;
}

int yf_node_setname(YF_node node, const char *name)
{
    assert(node != NULL);

    if (name == NULL) {
        free(node->name);
        node->name = NULL;
        return 0;
    }

    const size_t len = strlen(name);
    if (node->name == NULL) {
        node->name = malloc(1+len);
        if (node->name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }
    } else {
        const size_t cur_len = strlen(node->name);
        if (cur_len != len) {
            void *tmp = realloc(node->name, 1+len);
            if (tmp != NULL) {
                node->name = tmp;
            } else if (cur_len < len) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                return -1;
            }
        }
    }
    strcpy(node->name, name);
    return 0;
}

int yf_node_cmpname(YF_node node, const char *str)
{
    assert(node != NULL);

    const char *s1 = node->name == NULL ? "" : node->name;
    const char *s2 = str == NULL ? "" : str;
    return strcmp(s1, s2);
}

int yf_node_getobj(YF_node node, void **obj)
{
    assert(node != NULL);

    if (obj != NULL)
        *obj = node->obj;
    return node->nodeobj;
}

void yf_node_deinit(YF_node node)
{
    if (node == NULL)
        return;

    if (node->deinit != NULL)
        node->deinit(node->obj);

    yf_node_drop(node);
    yf_node_prune(node);
    free(node->name);
    free(node);
}

void yf_node_setobj(YF_node node, int nodeobj, void *obj,
                    void (*deinit)(void *obj))
{
    assert(node != NULL);
    assert(nodeobj >= YF_NODEOBJ_NONE && nodeobj <= YF_NODEOBJ_EFFECT);

    node->nodeobj = nodeobj;
    node->obj = obj;
    node->deinit = deinit;
}

YF_mat4 *yf_node_getwldxform(YF_node node)
{
    assert(node != NULL);
    return &node->wld_xform;
}

YF_mat4 *yf_node_getwldinv(YF_node node)
{
    assert(node != NULL);
    return &node->wld_inv;
}
