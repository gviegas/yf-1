/*
 * YF
 * cmpfn.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "cmpfn.h"

int yf_cmp(const void *ptr1, const void *ptr2)
{
    return (ptrdiff_t)ptr1 - (ptrdiff_t)ptr2;
}

int yf_cmpstr(const void *str1, const void *str2)
{
    assert(str1 != NULL && str2 != NULL);
    return strcmp(str1, str2);
}
