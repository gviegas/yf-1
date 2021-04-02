/*
 * YF
 * collection.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <yf/com/yf-hashset.h>
#include <yf/com/yf-error.h>

#include "collection.h"
#include "mesh.h"
#include "texture.h"
#include "font.h"

struct YF_collection_o {
  YF_hashset sets[YF_COLLRES_N];
  size_t n;
};

/* Type defining a managed resource. */
typedef struct {
  char *name;
  void *res;
} T_res;

/* Functions used by the collection sets. */
static size_t hash_res(const void *x);
static int cmp_res(const void *a, const void *b);
static int deinit_res(void *val, void *arg);

YF_collection yf_collection_init(const char *pathname) {
  YF_collection coll = calloc(1, sizeof(struct YF_collection_o));
  if (coll == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }

  for (size_t i = 0; i < YF_COLLRES_N; ++i) {
    coll->sets[i] = yf_hashset_init(hash_res, cmp_res);
    if (coll->sets[i] == NULL) {
      yf_collection_deinit(coll);
      return NULL;
    }
  }

  if (pathname != NULL) {
    /* TODO */
    assert(0);
  }

  return coll;
}

void *yf_collection_getres(YF_collection coll, int collres, const char *name) {
  assert(coll != NULL);
  assert(collres >= 0 && collres < YF_COLLRES_N);
  assert(name != NULL);

  const T_res key = {(char *)name, NULL};
  T_res *val = yf_hashset_search(coll->sets[collres], &key);
  return val != NULL ? val->res : NULL;
}

int yf_collection_manage(YF_collection coll, int collres, const char *name,
    void *res)
{
  assert(coll != NULL);
  assert(collres >= 0 && collres < YF_COLLRES_N);
  assert(name != NULL);
  assert(res != NULL);

  T_res *r = malloc(sizeof(T_res));
  if (r == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  r->name = malloc(1+strlen(name));
  if (r->name == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(r);
    return -1;
  }
  strcpy(r->name, name);
  r->res = res;

  if (yf_hashset_insert(coll->sets[collres], r) != 0) {
    free(r->name);
    free(r);
    return -1;
  }
  coll->n++;
  return 0;
}

int yf_collection_contains(YF_collection coll, int collres, const char *name) {
  assert(coll != NULL);
  assert(collres >= 0 && collres < YF_COLLRES_N);
  assert(name != NULL);

  const T_res res = {(char *)name, NULL};
  return yf_hashset_contains(coll->sets[collres], &res);
}

void yf_collection_deinit(YF_collection coll) {
  if (coll == NULL)
    return;

  for (size_t i = 0; i < YF_COLLRES_N; ++i) {
    if (coll->sets[i] == NULL)
      continue;
    yf_hashset_each(coll->sets[i], deinit_res, (void *)i);
    yf_hashset_deinit(coll->sets[i]);
  }

  free(coll);
}

static size_t hash_res(const void *x) {
  const char *str = ((const T_res *)x)->name;
  size_t hash = 0;
  while (*str != '\0')
    hash += (*str++)<<(hash&0xf);
  return hash ^ 0xb722a593;
}

static int cmp_res(const void *a, const void *b) {
  const char *s1 = ((const T_res *)a)->name;
  const char *s2 = ((const T_res *)b)->name;
  return strcmp(s1, s2);
}

static int deinit_res(void *val, void *arg) {
  T_res *res = val;

  switch ((size_t)arg) {
  case YF_COLLRES_MESH:
    yf_mesh_deinit(res->res);
    break;
  case YF_COLLRES_TEXTURE:
    yf_texture_deinit(res->res);
    break;
  case YF_COLLRES_FONT:
    yf_font_deinit(res->res);
    break;
  default:
    assert(0);
  }

  free(res->name);
  free(res);
  return 0;
}
