/*
 * YF
 * pubsub.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <assert.h>

#include "pubsub.h"

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
