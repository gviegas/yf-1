/*
 * YF
 * list.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "list.h"
#include "error.h"

/* Linked list's entry. */
typedef struct L_entry {
  struct L_entry *prev;
  struct L_entry *next;
  const void *val;
} L_entry;

struct YF_list_o {
  YF_cmpfn cmp;
  L_entry *first;
  size_t n;
};

YF_list yf_list_init(YF_cmpfn cmp) {
  YF_list list = malloc(sizeof(struct YF_list_o));
  if (list == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  if (cmp != NULL)
    list->cmp = cmp;
  else
    list->cmp = yf_cmp;
  list->first = NULL;
  list->n = 0;
  return list;
}

int yf_list_insert(YF_list list, const void *val) {
  assert(list != NULL);
  L_entry *e = malloc(sizeof(L_entry));
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
  ++list->n;
  return 0;
}

int yf_list_insertat(YF_list list, YF_iter *it, const void *val) {
  assert(list != NULL);
  if (it == NULL || YF_IT_ISNIL(*it)) {
    if (yf_list_insert(list, val) != 0)
      return -1;
    if (it != NULL) {
      it->data[0] = (size_t)list->first;
      it->data[1] = ~it->data[0];
    }
  } else {
    L_entry *e = malloc(sizeof(L_entry));
    if (e == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    L_entry *cur = (L_entry *)it->data[0];
    e->prev = cur;
    e->next = cur->next;
    e->val = val;
    cur->next = e;
    ++list->n;
    it->data[0] = (size_t)e;
  }
  return 0;
}

int yf_list_remove(YF_list list, const void *val) {
  assert(list != NULL);
  L_entry *e = list->first;
  while (e != NULL) {
    if (list->cmp(val, e->val) == 0) {
      if (e->prev != NULL)
        e->prev->next = e->next;
      else
        list->first = e->next;
      if (e->next != NULL)
        e->next->prev = e->prev;
      free(e);
      --list->n;
      return 0;
    }
    e = e->next;
  }
  yf_seterr(YF_ERR_NOTFND, __func__);
  return -1;
}

void *yf_list_removeat(YF_list list, YF_iter *it) {
  assert(list != NULL);
  if (list->first == NULL) {
    if (it != NULL)
      *it = YF_NILIT;
    return NULL;
  }
  L_entry *e = NULL;
  if (it == NULL || YF_IT_ISNIL(*it))
    e = list->first;
  else
    e = (L_entry *)it->data[0];
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
  --list->n;
  return r;
}

int yf_list_contains(YF_list list, const void *val) {
  assert(list != NULL);
  L_entry *e = list->first;
  while (e != NULL) {
    if (list->cmp(e->val, val) == 0)
      return 1;
    e = e->next;
  }
  return 0;
}

void *yf_list_next(YF_list list, YF_iter *it) {
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
    L_entry *e = (L_entry *)it->data[0];
    if (e->next != NULL) {
      it->data[0] = (size_t)e->next;
      r = (void *)e->next->val;
    } else {
      *it = YF_NILIT;
    }
  }
  return r;
}

void yf_list_each(YF_list list, int (*fn)(void *val, void *arg), void *arg) {
  assert(list != NULL);
  assert(fn != NULL);
  L_entry *e = list->first;
  while (e != NULL) {
    if (fn((void *)e->val, arg) != 0)
      break;
    e = e->next;
  }
}

size_t yf_list_getlen(YF_list list) {
  assert(list != NULL);
  return list->n;
}

void yf_list_clear(YF_list list) {
  assert(list != NULL);
  if (list->n == 0)
    return;
  L_entry *e = list->first;
  do {
    if (e->next == NULL) {
      free(e);
      break;
    }
    e = e->next;
    free(e->prev);
  } while (1);
  list->first = NULL;
  list->n = 0;
}

void yf_list_deinit(YF_list list) {
  if (list != NULL && list->n > 0) {
    L_entry *e = list->first;
    do {
      if (e->next == NULL) {
        free(e);
        break;
      }
      e = e->next;
      free(e->prev);
    } while (1);
  }
  free(list);
}
