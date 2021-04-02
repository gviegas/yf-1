/*
 * YF
 * hashset.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <time.h>
#include <assert.h>

#ifdef __linux__
# include <sys/random.h>
#endif

#include "hashset.h"
#include "list.h"
#include "error.h"

#undef YF_WMINBITS
#undef YF_WMAXBITS
#undef YF_WBITS
#define YF_WMINBITS 4
#define YF_WMAXBITS (sizeof(size_t) == 8 ? 48 : 29)
#define YF_WBITS (sizeof(size_t) * 8)

#undef YF_MINLOADF
#undef YF_MAXLOADF
#define YF_MINLOADF 0.25
#define YF_MAXLOADF 0.75

#undef YF_HASH
#define YF_HASH(res, a, x, b, w) (res = ((a)*(x)+(b)) >> (YF_WBITS-(w)))

#undef YF_LCG
#define YF_LCG(state, xn) do { \
  state = (0x5deece66dULL * (state) + 0xb) % 0x1000000000000ULL; \
  xn = (state) >> 16; } while (0)

struct YF_hashset_o {
  YF_hashfn hash;
  YF_cmpfn cmp;
  YF_list *buckets;
  unsigned bucket_w;
  size_t value_n;
  unsigned long long lcg_state;
  size_t a;
  size_t b;
};

/* Produces a seed for the random generator. */
static void make_seed(unsigned long long *seed);

/* Produces hashing function factors using a given state. */
static void make_factors(unsigned long long *state, size_t *a, size_t *b);

/* Rehashes a hashset. */
static int rehash(YF_hashset set, int down);

YF_hashset yf_hashset_init(YF_hashfn hash, YF_cmpfn cmp) {
  YF_hashset set = malloc(sizeof(struct YF_hashset_o));
  if (set == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  set->hash = hash != NULL ? hash : yf_hash;
  set->cmp = cmp != NULL ? cmp : yf_cmp;

  set->buckets = calloc(1 << YF_WMINBITS, sizeof *set->buckets);
  if (set->buckets == NULL) {
    free(set);
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  set->bucket_w = YF_WMINBITS;
  for (size_t i = 0; i < ((size_t)1 << set->bucket_w); ++i) {
    if ((set->buckets[i] = yf_list_init(set->cmp)) == NULL) {
      yf_hashset_deinit(set);
      return NULL;
    }
  }

  set->value_n = 0;
  make_seed(&set->lcg_state);
  make_factors(&set->lcg_state, &set->a, &set->b);
  return set;
}

int yf_hashset_insert(YF_hashset set, const void *val) {
  assert(set != NULL);

  size_t k, x = set->hash(val);
  YF_HASH(k, set->a, x, set->b, set->bucket_w);
  if (yf_list_contains(set->buckets[k], val)) {
    yf_seterr(YF_ERR_EXIST, __func__);
    return -1;
  }

  int r = yf_list_insert(set->buckets[k], val);
  if (r == 0) {
    ++set->value_n;
    if (set->bucket_w < YF_WMAXBITS) {
      if ((double)set->value_n / (double)(1 << set->bucket_w) > YF_MAXLOADF)
        rehash(set, 0);
    }
  }
  return r;
}

int yf_hashset_remove(YF_hashset set, const void *val) {
  assert(set != NULL);

  size_t k, x = set->hash(val);
  YF_HASH(k, set->a, x, set->b, set->bucket_w);

  int r = yf_list_remove(set->buckets[k], val);
  if (r == 0) {
    --set->value_n;
    if (set->bucket_w > YF_WMINBITS) {
      if ((double)set->value_n / (double)(1 << set->bucket_w) < YF_MINLOADF)
        rehash(set, 1);
    }
  }
  return r;
}

int yf_hashset_contains(YF_hashset set, const void *val) {
  assert(set != NULL);

  size_t k, x = set->hash(val);
  YF_HASH(k, set->a, x, set->b, set->bucket_w);
  return yf_list_contains(set->buckets[k], val);
}

void *yf_hashset_search(YF_hashset set, const void *val) {
  assert(set != NULL);

  size_t k, x = set->hash(val);
  YF_HASH(k, set->a, x, set->b, set->bucket_w);

  void *r = NULL;
  YF_iter it = YF_NILIT;
  do {
    r = yf_list_next(set->buckets[k], &it);
    if (YF_IT_ISNIL(it) || set->cmp(val, r) == 0)
      break;
  } while (1);

  return r;
}

void *yf_hashset_extract(YF_hashset set, YF_iter *it) {
  assert(set != NULL);

  void *r = NULL;
  YF_iter llit;
  if (it == NULL || YF_IT_ISNIL(*it)) {
    if (set->value_n != 0) {
      llit = YF_NILIT;
      for (size_t i = 0; i < ((size_t)1 << set->bucket_w); ++i) {
        r = yf_list_next(set->buckets[i], &llit);
        if (YF_IT_ISNIL(llit))
          continue;
        yf_list_remove(set->buckets[i], r);
        --set->value_n;
        if (set->bucket_w > YF_WMINBITS &&
            (double)set->value_n / (double)(1 << set->bucket_w) < YF_MINLOADF)
        {
          rehash(set, 1);
        }
        break;
      }
    }
  } else {
    const size_t i = it->data[0];
    r = (void *)it->data[1];
    yf_list_remove(set->buckets[i], r);
    --set->value_n;
    if (set->bucket_w > YF_WMINBITS &&
        (double)set->value_n / (double)(1 << set->bucket_w) < YF_MINLOADF)
    {
      rehash(set, 1);
    }
  }

  if (it != NULL)
    *it = YF_NILIT;
  return r;
}

void *yf_hashset_next(YF_hashset set, YF_iter *it) {
  assert(set != NULL);

  void *r = NULL;
  YF_iter llit;
  if (it == NULL || YF_IT_ISNIL(*it)) {
    if (set->value_n != 0) {
      llit = YF_NILIT;
      for (size_t i = 0; i < ((size_t)1 << set->bucket_w); ++i) {
        r = yf_list_next(set->buckets[i], &llit);
        if (YF_IT_ISNIL(llit))
          continue;
        if (it != NULL) {
          it->data[0] = i;
          it->data[1] = (size_t)r;
        }
        break;
      }
    } else if (it != NULL) {
      *it = YF_NILIT;
    }
  } else {
    void *val;
    int found_curr = 0;
    for (size_t i = it->data[0]; i < ((size_t)1 << set->bucket_w); ++i) {
      llit = YF_NILIT;
      do {
        val = yf_list_next(set->buckets[i], &llit);
        if (YF_IT_ISNIL(llit))
          break;
        if (found_curr) {
          it->data[0] = i;
          it->data[1] = (size_t)val;
          return val;
        }
        if (set->cmp(val, (const void *)it->data[1]) == 0)
          found_curr = 1;
      } while (1);
    }
    *it = YF_NILIT;
  }
  return r;
}

void yf_hashset_each(YF_hashset set, int (*callb)(void *val, void *arg),
    void *arg)
{
  assert(set != NULL);
  assert(callb != NULL);
  if (set->value_n == 0)
    return;

  YF_iter it;
  void *val;
  size_t n = 0;
  for (size_t i = 0; i < ((size_t)1 << set->bucket_w); ++i) {
    it = YF_NILIT;
    do {
      val = yf_list_next(set->buckets[i], &it);
      if (YF_IT_ISNIL(it))
        break;
      if (callb(val, arg) != 0 || ++n == set->value_n)
        return;
    } while (1);
  }
}

size_t yf_hashset_getlen(YF_hashset set) {
  assert(set != NULL);
  return set->value_n;
}

void yf_hashset_clear(YF_hashset set) {
  assert(set != NULL);
  if (set->value_n == 0)
    return;

  const size_t n = (size_t)1 << YF_WMINBITS;
  const size_t m = (size_t)1 << set->bucket_w;
  for (size_t i = 0; i < n; ++i)
    yf_list_clear(set->buckets[i]);
  for (size_t i = n; i < m; ++i)
    yf_list_deinit(set->buckets[i]);

  YF_list *tmp = realloc(set->buckets, n * sizeof *set->buckets);
  if (tmp != NULL)
    set->buckets = tmp;
  else
    memset(set->buckets+n, 0, (m-n) * sizeof *set->buckets);

  set->bucket_w = YF_WMINBITS;
  set->value_n = 0;
  make_factors(&set->lcg_state, &set->a, &set->b);
}

void yf_hashset_deinit(YF_hashset set) {
  if (set != NULL) {
    for (size_t i = 0; i < ((size_t)1 << set->bucket_w); ++i)
      yf_list_deinit(set->buckets[i]);
    free(set->buckets);
    free(set);
  }
}

static void make_seed(unsigned long long *seed) {
#ifdef __linux__
  if (getrandom(seed, sizeof *seed, 0) != sizeof *seed)
    *seed = time(NULL);
#else
  *seed = time(NULL);
#endif
  *seed &= (1ULL << 48) - 1;
}

static void make_factors(unsigned long long *state, size_t *a, size_t *b) {
  if (YF_WBITS == 64) {
    unsigned long long ah, al, bh, bl;
    YF_LCG(*state, ah);
    YF_LCG(*state, al);
    YF_LCG(*state, bh);
    YF_LCG(*state, bl);
    *a = (ah << 32) | al | 1;
    *b = (bh << 32) | bl;
  } else {
    unsigned long long ax, bx;
    YF_LCG(*state, ax);
    YF_LCG(*state, bx);
    *a = ax | 1;
    *b = bx;
  }
}

static int rehash(YF_hashset set, int down) {
  const unsigned w = down ? set->bucket_w-1 : set->bucket_w+1;
  size_t a, b;
  make_factors(&set->lcg_state, &a, &b);

  YF_list *tmp = calloc((size_t)1 << w, sizeof(YF_list));
  if (tmp == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  for (size_t i = 0; i < ((size_t)1 << w); ++i) {
    if ((tmp[i] = yf_list_init(set->cmp)) == NULL) {
      for (size_t j = 0; j < i; ++j)
        yf_list_deinit(tmp[j]);
      free(tmp);
      return -1;
    }
  }

  YF_iter it;
  void *val;
  size_t x, k;
  for (size_t i = 0; i < ((size_t)1 << set->bucket_w); ++i) {
    if (yf_list_getlen(set->buckets[i]) == 0)
      continue;
    it = YF_NILIT;
    do {
      val = yf_list_next(set->buckets[i], &it);
      if (YF_IT_ISNIL(it))
        break;
      x = set->hash(val);
      YF_HASH(k, a, x, b, w);
      if (yf_list_insert(tmp[k], val) != 0) {
        for (size_t j = 0; j < ((size_t)1 << w); ++j)
          yf_list_deinit(tmp[j]);
        free(tmp);
        return -1;
      }
    } while (1);
  }

  for (size_t i = 0; i < ((size_t)1 << set->bucket_w); ++i)
    yf_list_deinit(set->buckets[i]);
  free(set->buckets);

  set->buckets = tmp;
  set->bucket_w = w;
  set->a = a;
  set->b = b;
  return 0;
}
