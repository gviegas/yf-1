/*
 * YF
 * data-png.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <yf/com/yf-error.h>
#include <yf/core/yf-image.h>

#include "data-png.h"

#ifdef _DEFAULT_SOURCE
# include <endian.h>
#else
/* TODO */
# error "Invalid platform"
#endif

/* PNG file signature. */
static const uint8_t l_sign[8] = {137, 80, 78, 71, 13, 10, 26, 10};

int yf_loadpng(const char *pathname, YF_texdt *data) {
  assert(pathname != NULL);
  assert(data != NULL);

  FILE *file = fopen(pathname, "r");
  if (file == NULL) {
    yf_seterr(YF_ERR_NOFILE, __func__);
    return -1;
  }

  uint8_t sign[sizeof l_sign];
  if (fread(sign, 1, sizeof sign, file) != sizeof sign ||
      memcmp(l_sign, sign, sizeof sign) != 0)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    fclose(file);
    return -1;
  }

  /* TODO... */

  fclose(file);
  return 0;
}
