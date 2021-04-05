/*
 * YF
 * pubsub.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "pubsub.h"
#include "hashset.h"
#include "error.h"

/* TODO: Thread-safe. */

/* Publisher variables. */
typedef struct {
  const void *pub;
  unsigned pubsub_mask;
  YF_hashset subs;
} T_pub;

/* Subscriber variables. */
typedef struct {
  const void *sub;
  unsigned pubsub_mask;
  void (*callb)(void *, int, void *);
  void *arg;
} T_sub;

/* Hashset containing all publishers. */
static YF_hashset l_pubs = NULL;

/* Hashset functions for pub/sub. */
static size_t hash_ps(const void *x);
static int cmp_ps(const void *a, const void *b);

int yf_setpub(const void *pub, unsigned pubsub_mask)
{
  if (pub == NULL) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  if (l_pubs == NULL && (l_pubs = yf_hashset_init(hash_ps, cmp_ps)) == NULL)
    return -1;

  const T_pub key = {pub, 0, NULL};
  T_pub *val = yf_hashset_search(l_pubs, &key);

  /* removal */
  if (pubsub_mask == YF_PUBSUB_NONE) {
    if (val != NULL) {
      yf_hashset_remove(l_pubs, val);
      T_sub *sub;
      while ((sub = yf_hashset_extract(val->subs, NULL)) != NULL)
        free(sub);
      yf_hashset_deinit(val->subs);
      free(val);
    }
    return 0;
  }

  /* insertion/update */
  if (val == NULL) {
    if ((val = malloc(sizeof *val)) == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    val->pub = pub;
    val->subs = yf_hashset_init(hash_ps, cmp_ps);
    if (val->subs == NULL) {
      free(val);
      return -1;
    }
    if (yf_hashset_insert(l_pubs, val) != 0) {
      yf_hashset_deinit(val->subs);
      free(val);
      return -1;
    }
  }
  val->pubsub_mask = pubsub_mask;
  return 0;
}

unsigned yf_checkpub(const void *pub)
{
  assert(pub != NULL);

  if (l_pubs == NULL)
    return YF_PUBSUB_NONE;

  const T_pub key = {pub, 0, NULL};
  T_pub *val = yf_hashset_search(l_pubs, &key);

  return val != NULL ? val->pubsub_mask : YF_PUBSUB_NONE;
}

void yf_publish(const void *pub, int pubsub)
{
  assert(pub != NULL);
  assert(l_pubs != NULL);

  const T_pub key = {pub, 0, NULL};
  T_pub *val = yf_hashset_search(l_pubs, &key);
  if (val == NULL)
    return;

  YF_iter it = YF_NILIT;
  T_sub *sub;
  while ((sub = yf_hashset_next(val->subs, &it)) != NULL) {
    if (sub->pubsub_mask & pubsub)
      sub->callb((void *)pub, pubsub, sub->arg);
  }
}

int yf_subscribe(const void *pub, const void *sub, unsigned pubsub_mask,
    void (*callb)(void *pub, int pubsub, void *arg), void *arg)
{
  assert(pub != NULL);
  assert(pubsub_mask == YF_PUBSUB_NONE || callb != NULL);

  if (sub == NULL || l_pubs == NULL) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  const T_pub pk = {pub, 0, NULL};
  T_pub *pv = yf_hashset_search(l_pubs, &pk);
  if (pv == NULL)
    return -1;
  const T_sub sk = {sub, 0, NULL, NULL};
  T_sub *sv = yf_hashset_search(pv->subs, &sk);

  /* removal */
  if (pubsub_mask == YF_PUBSUB_NONE) {
    if (sv != NULL) {
      yf_hashset_remove(pv->subs, sv);
      free(sv);
    }
    return 0;
  }

  /* insertion/update */
  if (sv == NULL) {
    if ((sv = malloc(sizeof *sv)) == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    sv->sub = sub;
    if (yf_hashset_insert(pv->subs, sv) != 0) {
      free(sv);
      return -1;
    }
  }
  /* XXX: May want to compare pub/sub masks. */
  sv->pubsub_mask = pubsub_mask;
  sv->callb = callb;
  sv->arg = arg;
  return 0;
}

static size_t hash_ps(const void *x)
{
  const void *k = *(void **)x;
  return (size_t)k;
}

static int cmp_ps(const void *a, const void *b)
{
  const void *k1 = *(void **)a;
  const void *k2 = *(void **)b;
  return (ptrdiff_t)k1 - (ptrdiff_t)k2;
}
