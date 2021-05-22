/*
 * YF
 * hashfn.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdint.h>

#include "hashfn.h"

#if SIZE_MAX < 4294967295UL
# error "Unsupported system"
#elif SIZE_MAX < 18446744073709551615ULL
# define YF_HPRIME 16777619UL
# define YF_HSEED  2166136261UL
#else
# define YF_HPRIME 1099511628211ULL
# define YF_HSEED  14695981039346656037ULL
#endif

size_t yf_hash(const void *ptr)
{
    return (size_t)ptr;
}

size_t yf_hashstr(const void *str)
{
    const char *s = str;
    size_t hash = YF_HSEED;

    while (*s != '\0')
        hash = hash * YF_HPRIME ^ *s++;

    return hash;
}
