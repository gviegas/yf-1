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

#include "yf-dict.h"
#include "yf-error.h"

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

/* Key/value pair. */
typedef struct {
    const void *key;
    const void *val;
} pair_t;

/* List of key/value pairs. */
typedef struct {
    pair_t *pairs;
    size_t max_n;
    size_t cur_n;
} bucket_t;

struct yf_dict {
    yf_hashfn_t hash;
    yf_cmpfn_t cmp;
    bucket_t *buckets;
    size_t w;
    size_t count;
    unsigned long long lcg_state;
    size_t a;
    size_t b;
};

/* Hasher. */
#define YF_HASH(res, a, x, b, w) (res = ((a)*(x)+(b)) >> (YF_WBITS-(w)))

/* LCG. */
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
static int rehash(yf_dict_t *dict)
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
        void *tmp = realloc(dict->buckets, sizeof(bucket_t) * (1ULL<<new_w));
        if (tmp == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }

        dict->buckets = tmp;
        memset(&dict->buckets[1ULL<<dict->w], 0,
               sizeof(bucket_t) * (1ULL<<dict->w));

    } else {
        return 0;
    }

    const size_t ctrl_w = new_w > dict->w ? new_w : dict->w;
    size_t *ctrl = calloc(1ULL << ctrl_w, sizeof(size_t));
    if (ctrl == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    int r = 0;

cancel:

    for (size_t i = 0; i < 1ULL<<dict->w; i++) {
        if (ctrl[i] >= dict->buckets[i].cur_n)
            continue;

        size_t j = dict->buckets[i].cur_n;

        while (ctrl[i] < j) {
            pair_t *pair = dict->buckets[i].pairs+j-1;

            size_t k, x = dict->hash(pair->key);
            YF_HASH(k, dict->a, x, dict->b, new_w);

            bucket_t *bucket = dict->buckets+k;

            if (ctrl[k] == bucket->max_n) {
                const size_t new_n = ctrl[k] == 0 ? 1 : ctrl[k] << 1;
                void *tmp = realloc(bucket->pairs, sizeof(pair_t) * new_n);

                if (tmp == NULL) {
                    /* XXX: Dictionary is in an invalid state, this rehash
                       operation needs to be cancelled. */
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    r = -1;

                    if (new_w < dict->w) {
                        new_w = dict->w;
                    } else {
                        const size_t tmp = new_w;
                        new_w = dict->w;
                        dict->w = tmp;
                    }

                    memset(ctrl, 0, sizeof *ctrl * (1ULL<<ctrl_w));

                    /* rolling back to previous state does not require new
                       allocations, thus it is guaranteed to succeed */
                    goto cancel;
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
                const pair_t tmp = bucket->pairs[ctrl[k]];
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
        for (size_t i = 1ULL<<new_w; i < 1ULL<<dict->w; i++)
            free(dict->buckets[i].pairs);

        void *tmp = realloc(dict->buckets, sizeof(bucket_t) * (1ULL<<new_w));
        if (tmp != NULL)
            dict->buckets = tmp;
    }

    dict->w = new_w;

    return r;
}

yf_dict_t *yf_dict_init(yf_hashfn_t hash, yf_cmpfn_t cmp)
{
    yf_dict_t *dict = malloc(sizeof(yf_dict_t));
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

int yf_dict_insert(yf_dict_t *dict, const void *key, const void *val)
{
    assert(dict != NULL);

    size_t k, x = dict->hash(key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    bucket_t *bucket = dict->buckets+k;

    if (bucket->cur_n == bucket->max_n) {
        const size_t new_n = bucket->max_n == 0 ? 1 : bucket->max_n << 1;
        void *tmp = realloc(bucket->pairs, sizeof(pair_t) * new_n);
        if (tmp == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }

        bucket->pairs = tmp;
        bucket->max_n = new_n;
    }

    for (size_t i = 0; i < bucket->cur_n; i++) {
        if (dict->cmp(bucket->pairs[i].key, key) == 0) {
            yf_seterr(YF_ERR_EXIST, __func__);
            return -1;
        }
    }

    bucket->pairs[bucket->cur_n++] = (pair_t){key, val};
    dict->count++;

    rehash(dict);

    return 0;
}

void *yf_dict_remove(yf_dict_t *dict, const void *key)
{
    assert(dict != NULL);

    size_t k, x = dict->hash(key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    bucket_t *bucket = dict->buckets+k;
    size_t i = 0;

    for (; i < bucket->cur_n; i++) {
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
                sizeof(pair_t) * (bucket->cur_n - i - 1));

    bucket->cur_n--;
    dict->count--;

    rehash(dict);

    return (void *)val;
}

void *yf_dict_delete(yf_dict_t *dict, void **key)
{
    assert(dict != NULL);
    assert(key != NULL);

    size_t k, x = dict->hash(*key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    bucket_t *bucket = dict->buckets+k;
    size_t i = 0;

    for (; i < bucket->cur_n; i++) {
        if (dict->cmp(bucket->pairs[i].key, *key) == 0)
            break;
    }

    if (i == bucket->cur_n) {
        yf_seterr(YF_ERR_NOTFND, __func__);
        return NULL;
    }

    *key = (void *)bucket->pairs[i].key;
    const void *val = bucket->pairs[i].val;

    if (i+1 < bucket->cur_n)
        memmove(bucket->pairs+i, bucket->pairs+i+1,
                sizeof(pair_t) * (bucket->cur_n - i - 1));

    bucket->cur_n--;
    dict->count--;

    rehash(dict);

    return (void *)val;
}

void *yf_dict_replace(yf_dict_t *dict, const void *key, const void *val)
{
    assert(dict != NULL);

    size_t k, x = dict->hash(key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    bucket_t *bucket = dict->buckets+k;

    for (size_t i = 0; i < bucket->cur_n; i++) {
        if (dict->cmp(bucket->pairs[i].key, key) != 0)
            continue;

        const void *old_val = bucket->pairs[i].val;
        bucket->pairs[i].val = val;

        return (void *)old_val;
    }

    yf_seterr(YF_ERR_NOTFND, __func__);

    return NULL;
}

void *yf_dict_search(yf_dict_t *dict, const void *key)
{
    assert(dict != NULL);

    size_t k, x = dict->hash(key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    for (size_t i = 0; i < dict->buckets[k].cur_n; i++) {
        const pair_t *pair = &dict->buckets[k].pairs[i];

        if (dict->cmp(pair->key, key) == 0)
            return (void *)pair->val;
    }

    yf_seterr(YF_ERR_NOTFND, __func__);

    return NULL;
}

void *yf_dict_lookup(yf_dict_t *dict, void **key)
{
    assert(dict != NULL);
    assert(key != NULL);

    size_t k, x = dict->hash(*key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    for (size_t i = 0; i < dict->buckets[k].cur_n; i++) {
        const pair_t *pair = &dict->buckets[k].pairs[i];

        if (dict->cmp(pair->key, *key) == 0) {
            *key = (void *)pair->key;
            return (void *)pair->val;
        }
    }

    yf_seterr(YF_ERR_NOTFND, __func__);

    return NULL;
}

int yf_dict_contains(yf_dict_t *dict, const void *key)
{
    assert(dict != NULL);

    size_t k, x = dict->hash(key);
    YF_HASH(k, dict->a, x, dict->b, dict->w);

    if (dict->buckets[k].cur_n == 0)
        return 0;

    for (size_t i = 0; i < dict->buckets[k].cur_n; i++) {
        if (dict->cmp(dict->buckets[k].pairs[i].key, key) == 0)
            return 1;
    }

    return 0;
}

void *yf_dict_next(yf_dict_t *dict, yf_iter_t *it, void **key)
{
    assert(dict != NULL);

    pair_t *pair = NULL;

    if (it == NULL) {
        for (size_t i = 0; i < 1ULL<<dict->w; i++) {
            if (dict->buckets[i].cur_n == 0)
                continue;

            pair = dict->buckets[i].pairs;
            break;
        }

    } else if (YF_IT_ISNIL(*it)) {
        for (size_t i = 0; i < 1ULL<<dict->w; i++) {
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

            for (size_t i = bucket_i+1; i < 1ULL<<dict->w; i++) {
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

void yf_dict_each(yf_dict_t *dict,
                  int (*callb)(void *key, void *val, void *arg), void *arg)
{
    assert(dict != NULL);
    assert(callb != NULL);

    for (size_t i = 0; i < 1ULL<<dict->w; i++) {
        bucket_t *bucket = dict->buckets+i;

        for (size_t j = 0; j < bucket->cur_n; j++) {
            pair_t *pair = bucket->pairs+j;

            if (callb((void *)pair->key, (void *)pair->val, arg) != 0)
                return;
        }
    }
}

size_t yf_dict_getlen(yf_dict_t *dict)
{
    assert(dict != NULL);
    return dict->count;
}

void yf_dict_clear(yf_dict_t *dict)
{
    assert(dict != NULL);

    if (dict->count == 0)
        return;

    for (size_t i = 1ULL<<YF_WMINBITS; i < 1ULL<<dict->w; i++) {
        if (dict->buckets[i].max_n != 0) {
            free(dict->buckets[i].pairs);
            dict->count -= dict->buckets[i].cur_n;
        }
    }

    bucket_t *bucket = dict->buckets;

    while (dict->count > 0) {
        if (bucket->cur_n != 0) {
            dict->count -= bucket->cur_n;
            bucket->cur_n = 0;
        }

        bucket++;
    }

    dict->w = YF_WMINBITS;
    void *tmp = realloc(dict->buckets, sizeof(bucket_t) * (1ULL<<dict->w));

    if (tmp != NULL)
        dict->buckets = tmp;
}

void yf_dict_deinit(yf_dict_t *dict)
{
    if (dict == NULL)
        return;

    for (size_t i = 0; i < 1ULL<<dict->w; i++)
        free(dict->buckets[i].pairs);

    free(dict->buckets);
    free(dict);
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

void yf_print_dict(yf_dict_t *dict)
{
    printf("\ndict:\n w: %zu\n count: %zu\n lcg_state: %llu\n a: %zu\n b: %zu",
           dict->w, dict->count, dict->lcg_state, dict->a, dict->b);

    for (size_t i = 0; i < 1ULL<<dict->w; i++) {
        printf("\n buckets[%zu]:\n  max_n: %zu\n  cur_n: %zu", i,
               dict->buckets[i].max_n, dict->buckets[i].cur_n);
        for (size_t j = 0; j < dict->buckets[i].cur_n; j++)
            printf("\n  pairs[%zu]: %p/%p", j, dict->buckets[i].pairs[j].key,
                   dict->buckets[i].pairs[j].val);
    }

    puts("");
}

#endif
