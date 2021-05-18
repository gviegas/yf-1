/*
 * YF
 * dict.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef __linux__
# include <sys/random.h>
#endif

#include "dict.h"
#include "error.h"

#if SIZE_MAX < 4294967295UL
# error "Unsupported system"
#elif SIZE_MAX < 18446744073709551615ULL
# define YF_WMINBITS 4
# define YF_WMAXBITS 29
# define YF_WBITS    32
#else
# define YF_WMINBITS 4
# define YF_WMAXBITS 48
# define YF_WBITS    64
#endif

#define YF_MINLOADF 0.25
#define YF_MAXLOADF 0.75

typedef struct {
    const void *key;
    const void *val;
} T_pair;

typedef struct {
    T_pair *pairs;
    size_t max_n;
    size_t cur_n;
} T_bucket;

struct YF_dict_o {
    YF_hashfn hash;
    YF_cmpfn cmp;
    T_bucket *buckets;
    size_t w;
    size_t count;
    unsigned long long lcg_state;
    size_t a;
    size_t b;
};

#define YF_HASH(res, a, x, b, w) (res = ((a)*(x)+(b)) >> (YF_WBITS-(w)))

#define YF_LCG(state, xn) do { \
    state = (0x5deece66dULL * (state) + 0xb) % 0x1000000000000ULL; \
    xn = (state) >> 16; } while (0)

/* Produces a seed for the random generator. */
static void make_seed(unsigned long long *seed)
{
#ifdef __linux__
    if (getrandom(seed, sizeof *seed, 0) != sizeof *seed)
        *seed = time(NULL);
#else
    *seed = time(NULL);
#endif
    *seed &= (1ULL << 48) - 1;
}

/* Produces hashing function factors using a given state. */
static void make_factors(unsigned long long *state, size_t *a, size_t *b)
{
#if YF_WBITS == 64
    unsigned long long ah, al, bh, bl;
    YF_LCG(*state, ah);
    YF_LCG(*state, al);
    YF_LCG(*state, bh);
    YF_LCG(*state, bl);
    *a = (ah << 32) | al | 1;
    *b = (bh << 32) | bl;
#else
    unsigned long long ax, bx;
    YF_LCG(*state, ax);
    YF_LCG(*state, bx);
    *a = ax | 1;
    *b = bx;
#endif
}

YF_dict yf_dict_init(YF_hashfn hash, YF_cmpfn cmp)
{
    YF_dict dict = malloc(sizeof(struct YF_dict_o));

    if (dict == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    dict->hash = hash != NULL ? hash : yf_hash;
    dict->cmp = cmp != NULL ? cmp : yf_cmp;
    dict->w = YF_WMINBITS;
    dict->count = 0;
    dict->buckets = calloc(1 << dict->w, sizeof *dict->buckets);

    if (dict->buckets == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(dict);
        return NULL;
    }

    make_seed(&dict->lcg_state);
    make_factors(&dict->lcg_state, &dict->a, &dict->b);

    return dict;
}

int yf_dict_insert(YF_dict dict, const void *key, const void *val)
{
    assert(dict != NULL);

    size_t k, x = dict->hash(key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    T_bucket *bucket = dict->buckets+k;

    if (bucket->cur_n == bucket->max_n) {
        const size_t new_n = bucket->max_n == 0 ? 1 : bucket->max_n << 1;
        void *tmp = realloc(bucket->pairs, sizeof(T_pair) * new_n);

        if (tmp == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }

        bucket->pairs = tmp;
        bucket->max_n = new_n;
    }

    for (size_t i = 0; i < bucket->cur_n; ++i) {
        if (dict->cmp(bucket->pairs[i].key, key) == 0) {
            yf_seterr(YF_ERR_EXIST, __func__);
            return -1;
        }
    }

    bucket->pairs[bucket->cur_n++] = (T_pair){key, val};
    ++dict->count;

    // TODO: Check if rehash is needed

    return 0;
}

void *yf_dict_remove(YF_dict dict, const void *key)
{
    assert(dict != NULL);

    size_t k, x = dict->hash(key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    // TODO...
    return NULL;
}

int yf_dict_contains(YF_dict dict, const void *key)
{
    assert(dict != NULL);

    size_t k, x = dict->hash(key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    if (dict->buckets[k].cur_n == 0)
        return 0;

    for (size_t i = 0; i < dict->buckets[k].cur_n; ++i) {
        if (dict->cmp(dict->buckets[k].pairs[i].key, key) == 0)
            return 1;
    }

    return 0;
}

size_t yf_dict_getlen(YF_dict dict)
{
    assert(dict != NULL);
    return dict->count;
}

void yf_dict_deinit(YF_dict dict)
{
    assert(dict != NULL);

    for (size_t i = 0; i < 1ULL<<dict->w; ++i)
        free(dict->buckets[i].pairs);

    free(dict->buckets);
    free(dict);
}
