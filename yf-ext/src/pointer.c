/*
 * YF
 * pointer.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include "pointer.h"

/* The pointer coordinates. */
static YF_coord2 l_coord = {-1.0, -1.0};

YF_coord2 yf_pointer_getcoord(void) {
  return l_coord;
}

int yf_pointer_isvalid(void) {
#ifdef YF_USE_FLOAT64
  return l_coord.x >= 0.0 && l_coord.y >= 0.0;
#else
  return l_coord.x >= 0.0f && l_coord.y >= 0.0f;
#endif
}

void yf_pointer_setcoord(YF_coord2 coord) {
  l_coord = coord;
}

void yf_pointer_invalidate(void) {
#ifdef YF_USE_FLOAT64
  l_coord.x = l_coord.y = -1.0;
#else
  l_coord.x = l_coord.y = -1.0f;
#endif
}
