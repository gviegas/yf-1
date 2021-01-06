/*
 * YF
 * test-misc.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "data-sfnt.h"

int yf_test_misc(void) {
  return yf_loadsfnt("tmp/font.ttf");
}
