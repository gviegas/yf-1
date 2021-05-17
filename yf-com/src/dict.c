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
    void *list;
    size_t max_n;
    size_t cur_n;
    size_t last_i;
} T_bucket;

struct YF_dict_o {
    YF_hashfn hash;
    YF_cmpfn cmp;
    unsigned w;
    T_bucket *buckets;
    size_t bucket_n;
    size_t value_n;
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
