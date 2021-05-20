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

#ifdef YF_DEVEL
# include <stdio.h>
#endif

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

#define YF_MINLOADF 0.25f
#define YF_MAXLOADF 0.75f

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

/* Rehashes a dictionary. */
static int rehash(YF_dict dict)
{
    assert(dict != NULL);

    const float fac = (float)dict->count / (1ULL<<dict->w);
    size_t new_w;

    if (fac < YF_MINLOADF) {
        if (dict->w == YF_WMINBITS)
            return 0;

        new_w = dict->w - 1;

    } else if (fac > YF_MAXLOADF) {
        if (dict->w == YF_WMAXBITS)
            return 0;

        new_w = dict->w + 1;
        void *tmp = realloc(dict->buckets, sizeof(T_bucket) * (1ULL<<new_w));

        if (tmp == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }

        dict->buckets = tmp;
        memset(&dict->buckets[1ULL<<dict->w], 0,
               sizeof(T_bucket) * (1ULL<<dict->w));

    } else {
        return 0;
    }

    const size_t ctrl_w = new_w > dict->w ? new_w : dict->w;
    size_t *ctrl = calloc(1ULL << ctrl_w, sizeof(size_t));

    if (ctrl == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    for (size_t i = 0; i < 1ULL<<dict->w; ++i) {
        if (ctrl[i] >= dict->buckets[i].cur_n)
            continue;

        size_t j = dict->buckets[i].cur_n;

        while (ctrl[i] < j) {
            T_pair *pair = dict->buckets[i].pairs+j-1;

            size_t k, x = dict->hash(pair->key);
            YF_HASH(k, dict->a, x, dict->b, new_w);

            T_bucket *bucket = dict->buckets+k;

            if (ctrl[k] == bucket->max_n) {
                const size_t new_n = ctrl[k] == 0 ? 1 : ctrl[k] << 1;
                void *tmp = realloc(bucket->pairs, sizeof(T_pair) * new_n);

                if (tmp == NULL) {
                    /* XXX: Dictionary is in an invalid state */
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    free(ctrl);
                    return -1;
                }

                bucket->max_n = new_n;
                bucket->cur_n++;
                bucket->pairs = tmp;
                bucket->pairs[ctrl[k]] = *pair;
                ctrl[k]++;
                j--;

            } else if (ctrl[k] == bucket->cur_n) {
                bucket->cur_n++;
                bucket->pairs[ctrl[k]] = *pair;
                ctrl[k]++;
                j--;

            } else if (k != i || ctrl[k] != j-1) {
                const T_pair tmp = bucket->pairs[ctrl[k]];
                bucket->pairs[ctrl[k]] = *pair;
                *pair = tmp;
                ctrl[k]++;

            } else {
                ctrl[k]++;
                break;
            }
        }

        dict->buckets[i].cur_n = ctrl[i];
    }

    free(ctrl);

    if (new_w < dict->w) {
        for (size_t i = 1ULL<<new_w; i < 1ULL<<dict->w; ++i)
            free(dict->buckets[i].pairs);

        void *tmp = realloc(dict->buckets, sizeof(T_bucket) * (1ULL<<new_w));

        if (tmp != NULL)
            dict->buckets = tmp;
    }

    dict->w = new_w;

    return 0;
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

    rehash(dict);

    return 0;
}

void *yf_dict_remove(YF_dict dict, const void *key)
{
    assert(dict != NULL);

    size_t k, x = dict->hash(key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    T_bucket *bucket = dict->buckets+k;
    size_t i = 0;

    for (; i < bucket->cur_n; ++i) {
        if (dict->cmp(bucket->pairs[i].key, key) == 0)
            break;
    }

    if (i == bucket->cur_n) {
        yf_seterr(YF_ERR_NOTFND, __func__);
        return NULL;
    }

    const void *val = bucket->pairs[i].val;

    if (i+1 < bucket->cur_n)
        memmove(bucket->pairs+i, bucket->pairs+i+1,
                sizeof(T_pair) * (bucket->cur_n - i - 1));

    --bucket->cur_n;
    --dict->count;

    rehash(dict);

    return (void *)val;
}

void *yf_dict_replace(YF_dict dict, const void *key, const void *val)
{
    assert(dict != NULL);

    size_t k, x = dict->hash(key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    T_bucket *bucket = dict->buckets+k;

    for (size_t i = 0; i < bucket->cur_n; ++i) {
        if (dict->cmp(bucket->pairs[i].key, key) != 0)
            continue;

        const void *old_val = bucket->pairs[i].val;
        bucket->pairs[i].val = val;

        return (void *)old_val;
    }

    yf_seterr(YF_ERR_NOTFND, __func__);

    return NULL;
}

void *yf_dict_search(YF_dict dict, const void *key)
{
    assert(dict != NULL);

    size_t k, x = dict->hash(key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    for (size_t i = 0; i < dict->buckets[k].cur_n; ++i) {
        const T_pair *pair = &dict->buckets[k].pairs[i];

        if (dict->cmp(pair->key, key) == 0)
            return (void *)pair->val;
    }

    yf_seterr(YF_ERR_NOTFND, __func__);

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

void *yf_dict_next(YF_dict dict, YF_iter *it, void **key)
{
    assert(dict != NULL);

    T_pair *pair = NULL;

    if (it == NULL) {
        for (size_t i = 0; i < 1ULL<<dict->w; ++i) {
            if (dict->buckets[i].cur_n == 0)
                continue;

            pair = dict->buckets[i].pairs;
            break;
        }

    } else if (YF_IT_ISNIL(*it)) {
        for (size_t i = 0; i < 1ULL<<dict->w; ++i) {
            if (dict->buckets[i].cur_n == 0)
                continue;

            it->data[0] = i;
            it->data[1] = 0;
            pair = dict->buckets[i].pairs;
            break;
        }

    } else {
        const size_t bucket_i = it->data[0];
        const size_t pair_i = it->data[1];

        if (dict->buckets[bucket_i].cur_n > pair_i+1) {
            it->data[1]++;
            pair = &dict->buckets[bucket_i].pairs[pair_i+1];

        } else {
            *it = YF_NILIT;

            for (size_t i = bucket_i+1; i < 1ULL<<dict->w; ++i) {
                if (dict->buckets[i].cur_n == 0)
                    continue;

                it->data[0] = i;
                it->data[1] = 0;
                pair = dict->buckets[i].pairs;
                break;
            }
        }
    }

    if (pair != NULL) {
        if (key != NULL)
            *key = (void *)pair->key;

        return (void *)pair->val;
    }

    if (key != NULL)
        *key = NULL;

    return NULL;
}

void yf_dict_each(YF_dict dict, int (*callb)(void *key, void *val, void *arg),
                  void *arg)
{
    assert(dict != NULL);
    assert(callb != NULL);

    for (size_t i = 0; i < 1ULL<<dict->w; ++i) {
        T_bucket *bucket = dict->buckets+i;

        for (size_t j = 0; j < bucket->cur_n; ++j) {
            T_pair *pair = bucket->pairs+j;

            if (callb((void *)pair->key, (void *)pair->val, arg) != 0)
                return;
        }
    }
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

/*
 * DEVEL
 */

#ifdef YF_DEVEL

void yf_print_dict(YF_dict dict)
{
    printf("\ndict:\n w: %lu\n count: %lu\n lcg_state: %llu\n a: %lu\n b: %lu",
           dict->w, dict->count, dict->lcg_state, dict->a, dict->b);

    for (size_t i = 0; i < 1ULL<<dict->w; ++i) {
        printf("\n buckets[%lu]:\n  max_n: %lu\n  cur_n: %lu", i,
               dict->buckets[i].max_n, dict->buckets[i].cur_n);
        for (size_t j = 0; j < dict->buckets[i].cur_n; ++j)
            printf("\n  pairs[%lu]: %p/%p", j, dict->buckets[i].pairs[j].key,
                   dict->buckets[i].pairs[j].val);
    }

    puts("");
}

#endif
