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

/* Publisher variables. */
typedef struct {
  const void *pub;
  unsigned pubsub_mask;
  YF_hashset subs;
} L_pub;

/* Subscriber variables. */
typedef struct {
  const void *sub;
  unsigned pubsub_mask;
  void (*callb)(void *, unsigned, void *);
  void *arg;
} L_sub;

/* Hashset containing all publishers. */
static YF_hashset l_pubs = NULL;

/* Hashset functions for pub/sub. */
static size_t hash_ps(const void *x);
static int cmp_ps(const void *a, const void *b);

int yf_setpub(const void *pub, unsigned pubsub_mask) {
  /* TODO */
  assert(0);
}

unsigned yf_checkpub(const void *pub) {
  /* TODO */
  assert(0);
}

void yf_publish(const void *pub, int pubsub) {
  /* TODO */
  assert(0);
}

int yf_subscribe(const void *pub, const void *sub, unsigned pubsub_mask,
    void (*callb)(void *pub, int pubsub, void *arg), void *arg)
{
  /* TODO */
  assert(0);
}

static size_t hash_ps(const void *x) {
  const void *k = *(void **)x;
  return (size_t)k;
}

static int cmp_ps(const void *a, const void *b) {
  const void *k1 = *(void **)a;
  const void *k2 = *(void **)b;
  return (ptrdiff_t)k1 - (ptrdiff_t)k2;
}
