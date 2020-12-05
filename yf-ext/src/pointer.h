/*
 * YF
 * pointer.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_POINTER_H
#define YF_POINTER_H

#include "yf-pointer.h"

/* Sets the pointer coordinates. */
void yf_pointer_setcoord(YF_coord2 coord);

/* Invalidates the pointer coordinates. */
void yf_pointer_invalidate(void);

#endif /* YF_POINTER_H */
