/*
 * YF
 * collection.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include <yf/com/yf-hashset.h>
#include <yf/com/yf-error.h>

#include "collection.h"

struct YF_collection_o {
  YF_hashset sets[YF_COLLRES_N];
  size_t n;
};

/* Functions used by the collection sets. */
static size_t hash_res(const void *x);
static int cmp_res(const void *a, const void *b);

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
  /* TODO */
  assert(0);
}

void yf_collection_deinit(YF_collection coll) {
  /* TODO */
  assert(0);
}

static size_t hash_res(const void *x) {
  /* TODO */
  assert(0);
}

static int cmp_res(const void *a, const void *b) {
  /* TODO */
  assert(0);
}
