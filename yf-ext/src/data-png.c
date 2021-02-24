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

#define YF_PNG_MAKETYPE(c1, c2, c3, c4) \
  ((c1 << 24) | (c2 << 16) | (c3 << 8) | c4)

/* Image header. */
#define YF_PNG_IHDR YF_PNG_MAKETYPE('I', 'H', 'D', 'R')
typedef struct {
  uint32_t width;
  uint32_t height;
  uint8_t bit_depth;
  uint8_t color_type;
  uint8_t compression;
  uint8_t filter;
  uint8_t interlace;
} L_ihdr;
#define YF_PNG_IHDRSZ 13
static_assert(offsetof(L_ihdr, interlace) == YF_PNG_IHDRSZ-1, "!offsetof");

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

  struct { uint32_t len, type; } info;
  static_assert(sizeof info == 8);

  /* IHDR */
  if (fread(&info, 1, sizeof info, file) != sizeof info) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    fclose(file);
    return -1;
  }
  info.len = be32toh(info.len);
  info.type = be32toh(info.type);
  L_ihdr ihdr;
  if (info.type != YF_PNG_IHDR || info.len != YF_PNG_IHDRSZ ||
      fread(&ihdr, 1, YF_PNG_IHDRSZ, file) != YF_PNG_IHDRSZ)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    fclose(file);
    return -1;
  }
  ihdr.width = be32toh(ihdr.width);
  ihdr.height = be32toh(ihdr.height);

  //////////
  printf("\n%u %c%c%c%c\n", info.len, info.type & 0xff, (info.type >> 8) & 0xff,
      (info.type >> 16) & 0xff, (info.type >> 24) & 0xff);
  printf("\n%ux%u, %x, %x, %x, %x, %x\n", ihdr.width, ihdr.height,
      ihdr.bit_depth, ihdr.color_type, ihdr.compression, ihdr.filter,
      ihdr.interlace);
  //////////

  /* TODO... */

  fclose(file);
  return 0;
}
