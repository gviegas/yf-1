/*
 * YF
 * platform.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <assert.h>

#include "platform.h"

/* The platform in use. */
static int l_plat = YF_PLATFORM_NONE;

void yf_setplatform(int platform) {
  assert(l_plat == YF_PLATFORM_NONE || platform == l_plat);
  l_plat = platform;
}

int yf_getplatform(void) {
  return l_plat;
}
