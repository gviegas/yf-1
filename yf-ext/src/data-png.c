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

/* Chunk. */
typedef struct {
  uint32_t len;
  uint32_t type;
  uint8_t var[];
} L_chunk;
#define YF_PNG_VARSZ 4096
static_assert(sizeof(L_chunk) == 8, "!sizeof");

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

/* Palette. */
#define YF_PNG_PLTE YF_PNG_MAKETYPE('P', 'L', 'T', 'E')

/* Image data. */
#define YF_PNG_IDAT YF_PNG_MAKETYPE('I', 'D', 'A', 'T')

/* Image trailer. */
#define YF_PNG_IEND YF_PNG_MAKETYPE('I', 'E', 'N', 'D')

/* PNG file signature. */
static const uint8_t l_sign[8] = {137, 80, 78, 71, 13, 10, 26, 10};

/* CRC table. */
static uint32_t l_crctab[256] = {0};
/* TODO: Atomic. */
static int l_crcdone = 0;

#define YF_PNG_INITCRC() do { \
  if (!l_crcdone) { \
    for (uint32_t i = 0; i < 256; ++i) { \
      uint32_t crc = i, j = 8; \
      while (j--) \
        crc = (crc & 1) ? (0xedb88320 ^ (crc >> 1)) : (crc >> 1); \
      l_crctab[i] = crc; \
    } \
    l_crcdone = 1; \
  } } while (0)

#define YF_PNG_CALCCRC(crc, data, len) do { \
  crc = ~0U; \
  for (size_t i = 0; i < (len); ++i) \
    crc = l_crctab[((crc) ^ (data)[i]) & 0xff] ^ ((crc) >> 8); \
  crc ^= ~0U; } while (0)

int yf_loadpng(const char *pathname, YF_texdt *data) {
  assert(pathname != NULL);
  assert(data != NULL);

  FILE *file = fopen(pathname, "r");
  if (file == NULL) {
    yf_seterr(YF_ERR_NOFILE, __func__);
    return -1;
  }

  uint8_t sign[sizeof l_sign];
  if (fread(sign, sizeof sign, 1, file) < 1 ||
      memcmp(l_sign, sign, sizeof sign) != 0)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    fclose(file);
    return -1;
  }

  L_chunk *chunk = malloc(sizeof(L_chunk)+YF_PNG_VARSZ);
  if (chunk == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    fclose(file);
    return -1;
  }
  size_t var_sz = YF_PNG_VARSZ;
  uint32_t len, type, crc, crc_res;

  YF_PNG_INITCRC();

  /* IHDR */
  if (fread(chunk, sizeof(L_chunk), 1, file) < 1) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    free(chunk);
    fclose(file);
    return -1;
  }
  len = be32toh(chunk->len);
  type = be32toh(chunk->type);

  static_assert(YF_PNG_IHDRSZ <= YF_PNG_VARSZ);

  if (type != YF_PNG_IHDR || len != YF_PNG_IHDRSZ ||
      fread(chunk->var, len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    free(chunk);
    fclose(file);
    return -1;
  }

  if (fread(&crc, sizeof crc, 1, file) < 1) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    free(chunk);
    fclose(file);
    return -1;
  }
  crc = be32toh(crc);
  YF_PNG_CALCCRC(crc_res, (uint8_t *)(&chunk->type), len+sizeof chunk->type);
  if (crc != crc_res) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    free(chunk);
    fclose(file);
    return -1;
  }

  L_ihdr ihdr;
  memcpy(&ihdr, chunk->var, len);
  ihdr.width = be32toh(ihdr.width);
  ihdr.height = be32toh(ihdr.height);

  if (ihdr.width == 0 || ihdr.height == 0 ||
      (ihdr.bit_depth != 1 && ihdr.bit_depth != 2 && ihdr.bit_depth != 4 &&
        ihdr.bit_depth != 8 && ihdr.bit_depth != 16) ||
      (ihdr.color_type != 0 && ihdr.color_type != 2 && ihdr.color_type != 3 &&
        ihdr.color_type != 4 && ihdr.color_type != 6) ||
      ihdr.compression != 0 || ihdr.filter != 0 || ihdr.interlace > 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    free(chunk);
    fclose(file);
    return -1;
  }

  /* chunk processing */
  uint8_t *plte = NULL;
  size_t plte_sz = 0;
  uint8_t *idat = NULL;
  size_t idat_sz = 0;

  do {
    if (fread(chunk, sizeof(L_chunk), 1, file) < 1) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      free(chunk);
      fclose(file);
      return -1;
    }
    const int critical = !(chunk->type & 0x20);
    len = be32toh(chunk->len);
    type = be32toh(chunk->type);

    if (len > var_sz) {
      const size_t new_sz = sizeof(L_chunk)+len;
      void *tmp = realloc(chunk, new_sz);
      if (tmp != NULL) {
        chunk = tmp;
        var_sz = new_sz;
      } else if (critical) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(idat);
        free(plte);
        free(chunk);
        fclose(file);
        return -1;
      } else if (fseek(file, len+sizeof crc, SEEK_CUR) != 0) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(idat);
        free(plte);
        free(chunk);
        fclose(file);
        return -1;
      } else {
        continue;
      }
    }

    /* IEND */
    if (type == YF_PNG_IEND) {
      break;

    /* no data */
    } else if (len == 0) {

    /* PLTE */
    } else if (type == YF_PNG_PLTE) {
      if (ihdr.color_type == 0 || ihdr.color_type == 4 || len % 3 != 0 ||
          plte != NULL || idat != NULL)
      {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(idat);
        free(plte);
        free(chunk);
        fclose(file);
        return -1;
      }
      if (fread(chunk->var, len, 1, file) < 1) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(plte);
        free(chunk);
        fclose(file);
        return -1;
      }
      plte = malloc(len);
      if (plte == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        fclose(file);
        return -1;
      }
      memcpy(plte, chunk->var, len);
      plte_sz = len;

    /* IDAT */
    } else if (type == YF_PNG_IDAT) {
      if (fread(chunk->var, len, 1, file) < 1) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(idat);
        free(plte);
        free(chunk);
        fclose(file);
        return -1;
      }
      void *tmp = realloc(idat, idat_sz+len);
      if (tmp == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(idat);
        free(plte);
        free(chunk);
        fclose(file);
        return -1;
      }
      idat = tmp;
      memcpy(idat+idat_sz, chunk->var, len);
      idat_sz += len;

    /* unsupported */
    } else if (critical) {
      yf_seterr(YF_ERR_UNSUP, __func__);
      free(idat);
      free(plte);
      free(chunk);
      fclose(file);
      return -1;

    /* unknown */
    } else {
      if (fseek(file, len+sizeof crc, SEEK_CUR) != 0) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(idat);
        free(plte);
        free(chunk);
        fclose(file);
        return -1;
      }
      continue;
    }

    if (fread(&crc, sizeof crc, 1, file) < 1) {
      yf_seterr(YF_ERR_INVFILE, __func__);
      free(idat);
      free(plte);
      free(chunk);
      fclose(file);
      return -1;
    }
    crc = be32toh(crc);
    YF_PNG_CALCCRC(crc_res, (uint8_t *)(&chunk->type), len+sizeof chunk->type);
    if (crc != crc_res) {
      yf_seterr(YF_ERR_INVFILE, __func__);
      free(idat);
      free(plte);
      free(chunk);
      fclose(file);
      return -1;
    }
  } while (1);

  free(chunk);
  fclose(file);

  if (idat == NULL) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    free(plte);
    return -1;
  }

  /* data processing */
  switch (ihdr.color_type) {
    case 0:
      /* greyscale */
      /* TODO */
      break;

    case 2:
      /* rgb */
      switch (ihdr.bit_depth) {
        case 8:
        case 16:
          /* TODO */
          break;
        default:
          yf_seterr(YF_ERR_INVFILE, __func__);
          free(idat);
          free(plte);
          return -1;
      }
      break;

    case 3:
      /* palette indices */
      if (plte == NULL) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(idat);
        return -1;
      }
      switch (ihdr.bit_depth) {
        case 1:
        case 2:
        case 4:
        case 8:
          /* TODO */
          break;
        default:
          yf_seterr(YF_ERR_INVFILE, __func__);
          free(idat);
          free(plte);
          return -1;
      }
      break;

    case 4:
      /* greyscale w/ alpha */
      switch (ihdr.bit_depth) {
        case 8:
        case 16:
          /* TODO */
          break;
        default:
          yf_seterr(YF_ERR_INVFILE, __func__);
          free(idat);
          free(plte);
          return -1;
      }
      break;

    case 6:
      /* rgba */
      switch (ihdr.bit_depth) {
        case 8:
        case 16:
          /* TODO */
          break;
        default:
          yf_seterr(YF_ERR_INVFILE, __func__);
          free(idat);
          free(plte);
          return -1;
      }
      break;
  }

  /* TODO... */

  free(idat);
  free(plte);
  return 0;
}
