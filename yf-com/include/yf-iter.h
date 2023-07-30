/*
 * YF
 * yf-iter.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_ITER_H
#define YF_YF_ITER_H

#include <stdint.h>
#include <string.h>

/**
 * Type defining an iterator.
 */
typedef struct yf_iter {
    size_t data[2];
} yf_iter_t;

/**
 * The nil iterator.
 */
#define YF_NILIT (yf_iter_t){{SIZE_MAX, SIZE_MAX}}

/**
 * Checks whether or not two iterators are equal.
 *
 * @param it1: The first iterator.
 * @param it2: The second iterator.
 * @return: If 'it1' and 'it2' are equal, returns a non-zero value. Otherwise,
 *  zero is returned.
 */
#define YF_IT_ISEQ(it1, it2) \
    (memcmp((it1).data, (it2).data, sizeof (it1).data) == 0)

/**
 * Checks whether or not a given iterator is nil.
 *
 * @param it: The iterator.
 * @return: If 'it' is the nil iterator, returns a non-zero value. Otherwise,
 *  zero is returned.
 */
#define YF_IT_ISNIL(it) (YF_IT_ISEQ(it, YF_NILIT))

#endif /* YF_YF_ITER_H */
