/*
 * YF
 * hashfn.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdint.h>
#include <stdarg.h>
#include <assert.h>

#include "yf-hashfn.h"

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
    assert(str != NULL);

    const char *s = str;
    size_t hash = YF_HSEED;

    while (*s != '\0')
        hash = hash * YF_HPRIME ^ *s++;

    return hash;
}

size_t yf_hashv(const void *buf, size_t len, ...)
{
    assert(buf != NULL);

    va_list ap;
    const char *b = buf;
    size_t n = len;
    size_t hash = YF_HSEED;

    va_start(ap, len);

    while (1) {
        while (n--)
            hash = hash * YF_HPRIME ^ *b++;

        b = va_arg(ap, void *);

        if (b == NULL)
            break;

        n = va_arg(ap, size_t);
    }

    va_end(ap);

    return hash;
}
