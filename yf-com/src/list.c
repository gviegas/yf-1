/*
 * YF
 * list.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf-list.h"
#include "yf-error.h"

/* Linked list's entry. */
typedef struct entry entry_t;
struct entry {
    entry_t *prev;
    entry_t *next;
    const void *val;
};

struct yf_list {
    yf_cmpfn_t cmp;
    entry_t *first;
    size_t n;
};

yf_list_t *yf_list_init(yf_cmpfn_t cmp)
{
    yf_list_t *list = malloc(sizeof(yf_list_t));
    if (list == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    list->cmp = cmp != NULL ? cmp : yf_cmp;
    list->first = NULL;
    list->n = 0;

    return list;
}

int yf_list_insert(yf_list_t *list, const void *val)
{
    assert(list != NULL);

    entry_t *e = malloc(sizeof(entry_t));
    if (e == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    e->val = val;
    e->prev = NULL;
    e->next = list->first;

    if (list->first != NULL)
        list->first->prev = e;

    list->first = e;
    list->n++;

    return 0;
}

int yf_list_insertat(yf_list_t *list, yf_iter_t *it, const void *val)
{
    assert(list != NULL);

    if (it == NULL) {
        if (yf_list_insert(list, val) != 0)
            return -1;

    } else if (YF_IT_ISNIL(*it)) {
        if (yf_list_insert(list, val) != 0)
            return -1;

        it->data[0] = (size_t)list->first;
        it->data[1] = ~it->data[0];

    } else {
        entry_t *e = malloc(sizeof(entry_t));
        if (e == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }

        entry_t *cur = (entry_t *)it->data[0];
        e->prev = cur;
        e->next = cur->next;
        e->val = val;
        cur->next = e;
        list->n++;
        it->data[0] = (size_t)e;
    }

    return 0;
}

int yf_list_remove(yf_list_t *list, const void *val)
{
    assert(list != NULL);

    entry_t *e = list->first;

    while (e != NULL) {
        if (list->cmp(val, e->val) == 0) {
            if (e->prev != NULL)
                e->prev->next = e->next;
            else
                list->first = e->next;

            if (e->next != NULL)
                e->next->prev = e->prev;

            free(e);
            list->n--;

            return 0;
        }

        e = e->next;
    }

    yf_seterr(YF_ERR_NOTFND, __func__);

    return -1;
}

void *yf_list_removeat(yf_list_t *list, yf_iter_t *it)
{
    assert(list != NULL);

    if (list->first == NULL) {
        if (it != NULL)
            *it = YF_NILIT;

        return NULL;
    }

    entry_t *e = NULL;

    if (it == NULL || YF_IT_ISNIL(*it))
        e = list->first;
    else
        e = (entry_t *)it->data[0];

    if (e->prev != NULL)
        e->prev->next = e->next;
    else
        list->first = e->next;

    if (e->next != NULL) {
        e->next->prev = e->prev;

        if (it != NULL)
            it->data[0] = (size_t)e->next;

    } else if (it != NULL) {
        *it = YF_NILIT;
    }

    void *r = (void *)e->val;
    free(e);
    list->n--;

    return r;
}

int yf_list_contains(yf_list_t *list, const void *val)
{
    assert(list != NULL);

    entry_t *e = list->first;

    while (e != NULL) {
        if (list->cmp(e->val, val) == 0)
            return 1;

        e = e->next;
    }

    return 0;
}

void *yf_list_next(yf_list_t *list, yf_iter_t *it)
{
    assert(list != NULL);

    void *r = NULL;

    if (it == NULL) {
        if (list->first != NULL)
            r = (void *)list->first->val;

    } else if (YF_IT_ISNIL(*it)) {
        if (list->first != NULL) {
            it->data[0] = (size_t)list->first;
            it->data[1] = ~it->data[1];
            r = (void *)list->first->val;
        }

    } else {
        entry_t *e = (entry_t *)it->data[0];

        if (e->next != NULL) {
            it->data[0] = (size_t)e->next;
            r = (void *)e->next->val;
        } else {
            *it = YF_NILIT;
        }
    }

    return r;
}

void yf_list_each(yf_list_t *list,
                  int (*callb)(void *val, void *arg), void *arg)
{
    assert(list != NULL);
    assert(callb != NULL);

    entry_t *e = list->first;

    while (e != NULL) {
        if (callb((void *)e->val, arg) != 0)
            break;

        e = e->next;
    }
}

size_t yf_list_getlen(yf_list_t *list)
{
    assert(list != NULL);
    return list->n;
}

void yf_list_clear(yf_list_t *list)
{
    assert(list != NULL);

    if (list->n == 0)
        return;

    entry_t *e = list->first;

    while (1) {
        if (e->next == NULL) {
            free(e);
            break;
        }

        e = e->next;
        free(e->prev);
    }

    list->first = NULL;
    list->n = 0;
}

void yf_list_deinit(yf_list_t *list)
{
    if (list != NULL && list->n > 0) {
        entry_t *e = list->first;

        while (1) {
            if (e->next == NULL) {
                free(e);
                break;
            }

            e = e->next;
            free(e->prev);
        }
    }

    free(list);
}
