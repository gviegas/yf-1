/*
 * YF
 * iter.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_ITER_H
#define YF_ITER_H

#include <string.h>
#include <stdint.h>

/* Type defining an iterator. */
typedef struct {
  size_t data[2];
} YF_iter;

/* The nil iterator. */
#define YF_NILIT (YF_iter){{SIZE_MAX, SIZE_MAX}}

/* Checks whether or not two iterators are equal. */
#define YF_IT_ISEQ(it1, it2) \
  (memcmp((it1).data, (it2).data, sizeof (it1).data) == 0)

/* Checks whether or not a given iterator is nil. */
#define YF_IT_ISNIL(it) (YF_IT_ISEQ(it, YF_NILIT))

#endif /* YF_ITER_H */
