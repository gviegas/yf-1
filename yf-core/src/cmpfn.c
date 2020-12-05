/*
 * YF
 * cmpfn.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stddef.h>

#include "cmpfn.h"

int yf_cmp(const void *ptr1, const void *ptr2) {
  return (ptrdiff_t)ptr1 - (ptrdiff_t)ptr2;
}
