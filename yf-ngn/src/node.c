/*
 * YF
 * node.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "node.h"

/* TODO: Redesign this.
   It's too big and things like TRS just don't belong here. */
struct yf_node {
    yf_node_t *parent;
    yf_node_t *prev_sibl;
    yf_node_t *next_sibl;
    yf_node_t *child;
    size_t n;

    yf_mat4_t xform;
    yf_vec3_t t;
    yf_vec4_t r;
    yf_vec3_t s;
    int pending;
    char *name;

    int nodeobj;
    void *obj;
    void (*deinit)(void *);

    yf_mat4_t wld_xform;
    yf_mat4_t wld_inv;
    yf_mat4_t wld_norm;
};

yf_node_t *yf_node_init(void)
{
    yf_node_t *node = malloc(sizeof(yf_node_t));
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
    yf_vec3_set(node->t, 0.0f);
    yf_vec3_set(node->r, 0.0f);
    node->r[3] = 1.0f;
    yf_vec3_set(node->s, 1.0f);
    node->pending = 0;
    node->name = NULL;

    node->nodeobj = YF_NODEOBJ_NONE;
    node->obj = NULL;
    node->deinit = NULL;

    yf_mat4_iden(node->wld_xform);
    yf_mat4_iden(node->wld_inv);
    yf_mat4_iden(node->wld_norm);

    return node;
}

void yf_node_insert(yf_node_t *node, yf_node_t *child)
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

    yf_node_t *aux = node;
    do
        aux->n += child->n;
    while ((aux = aux->parent) != NULL);
}

void yf_node_drop(yf_node_t *node)
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

    yf_node_t *aux = node->parent;
    do
        aux->n -= node->n;
    while ((aux = aux->parent) != NULL);

    node->parent = NULL;
    node->prev_sibl = NULL;
    node->next_sibl = NULL;
}

void yf_node_prune(yf_node_t *node)
{
    assert(node != NULL);

    if (node->child == NULL)
        return;

    yf_node_t *cur = node->child;
    yf_node_t *next;
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

    yf_node_t *aux = node;
    do
        aux->n -= n;
    while ((aux = aux->parent) != NULL);
}

int yf_node_traverse(yf_node_t *node,
                     int (*fn)(yf_node_t *descendant, void *arg),
                     void *arg)
{
    assert(node != NULL);
    assert(fn != NULL);

    if (node->child == NULL)
        return 0;

    yf_node_t **queue = malloc(node->n * sizeof(yf_node_t *));
    if (queue == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    queue[0] = node;
    yf_node_t *next = NULL;
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

int yf_node_descends(yf_node_t *node, yf_node_t *ancestor)
{
    assert(node != NULL);
    assert(ancestor != NULL);
    assert(node != ancestor);

    yf_node_t *anc = node->parent;
    while (anc != NULL) {
        if (anc != ancestor)
            anc = anc->parent;
        else
            return 1;
    }
    return 0;
}

int yf_node_isleaf(yf_node_t *node)
{
    assert(node != NULL);
    return node->child == NULL;
}

int yf_node_isroot(yf_node_t *node)
{
    assert(node != NULL);
    return node->parent == NULL;
}

yf_node_t *yf_node_getparent(yf_node_t *node)
{
    assert(node != NULL);
    return node->parent;
}

size_t yf_node_getlen(yf_node_t *node)
{
    assert(node != NULL);
    return node->n;
}

yf_mat4_t *yf_node_getxform(yf_node_t *node)
{
    assert(node != NULL);

    if (node->pending) {
        node->pending = 0;
        yf_mat4_t tr, t, r, s;
        yf_mat4_xlate(t, node->t[0], node->t[1], node->t[2]);
        yf_mat4_rotq(r, node->r);
        yf_mat4_scale(s, node->s[0], node->s[1], node->s[2]);
        yf_mat4_mul(tr, t, r);
        /* XXX: 'xform' is overwritten. */
        yf_mat4_mul(node->xform, tr, s);
    }
    return &node->xform;
}

yf_vec3_t *yf_node_gett(yf_node_t *node)
{
    assert(node != NULL);

    node->pending = 1;
    return &node->t;
}

yf_vec4_t *yf_node_getr(yf_node_t *node)
{
    assert(node != NULL);

    node->pending = 1;
    return &node->r;
}

yf_vec3_t *yf_node_gets(yf_node_t *node)
{
    assert(node != NULL);

    node->pending = 1;
    return &node->s;
}

char *yf_node_getname(yf_node_t *node, char *dst, size_t *n)
{
    assert(node != NULL);
    assert(n != NULL);
    assert(dst == NULL || *n > 0);

    if (dst == NULL) {
        *n = node->name == NULL ? 1 : strlen(node->name) + 1;
        return NULL;
    }

    if (node->name == NULL) {
        dst[0] = '\0';
        *n = 1;
        return dst;
    }

    const size_t name_n = strlen(node->name) + 1;
    if (name_n <= *n) {
        *n = name_n;
        return strcpy(dst, node->name);
    }
    *n = name_n;
    return NULL;
}

int yf_node_setname(yf_node_t *node, const char *name)
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

int yf_node_cmpname(yf_node_t *node, const char *str)
{
    assert(node != NULL);

    const char *s1 = node->name == NULL ? "" : node->name;
    const char *s2 = str == NULL ? "" : str;
    return strcmp(s1, s2);
}

int yf_node_getobj(yf_node_t *node, void **obj)
{
    assert(node != NULL);

    if (obj != NULL)
        *obj = node->obj;
    return node->nodeobj;
}

void yf_node_deinit(yf_node_t *node)
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

void yf_node_setobj(yf_node_t *node, int nodeobj, void *obj,
                    void (*deinit)(void *obj))
{
    assert(node != NULL);
    assert(nodeobj >= YF_NODEOBJ_NONE && nodeobj <= YF_NODEOBJ_EFFECT);

    node->nodeobj = nodeobj;
    node->obj = obj;
    node->deinit = deinit;
}

yf_mat4_t *yf_node_getwldxform(yf_node_t *node)
{
    assert(node != NULL);
    return &node->wld_xform;
}

yf_mat4_t *yf_node_getwldinv(yf_node_t *node)
{
    assert(node != NULL);
    return &node->wld_inv;
}

yf_mat4_t *yf_node_getwldnorm(yf_node_t *node)
{
    assert(node != NULL);
    return &node->wld_norm;
}
