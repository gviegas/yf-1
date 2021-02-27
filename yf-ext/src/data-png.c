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

#include <yf/com/yf-util.h>
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

/* Processed PNG chunks. */
typedef struct {
  L_ihdr *ihdr;
  uint8_t *plte;
  size_t plte_sz;
  uint8_t *idat;
  size_t idat_sz;
} L_png;

/* Loads texture data from processed chunks. */
static int load_texdt(const L_png *png, YF_texdt *data);

/* Generates Huffman codes from a sequence of code lengths. */
static int gen_codes(const uint8_t *lengths, size_t length_n, uint32_t *codes);

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
  const L_png png = {
    .ihdr = &ihdr,
    .plte = plte,
    .plte_sz = plte_sz,
    .idat = idat,
    .idat_sz = idat_sz
  };
  int r;

  switch (ihdr.color_type) {
    case 0:
      /* greyscale */
      r = load_texdt(&png, data);
      break;

    case 2:
      /* rgb */
      switch (ihdr.bit_depth) {
        case 8:
        case 16:
          r = load_texdt(&png, data);
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
          r = load_texdt(&png, data);
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
          r = load_texdt(&png, data);
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
          r = load_texdt(&png, data);
          break;
        default:
          yf_seterr(YF_ERR_INVFILE, __func__);
          free(idat);
          free(plte);
          return -1;
      }
      break;
  }

  free(idat);
  free(plte);
  return r;
}

static int load_texdt(const L_png *png, YF_texdt *data) {
  assert(png != NULL);
  assert(data != NULL);

  struct {
    uint8_t cm:4, cinfo:4;
    uint8_t fcheck:5, fdict:1, flevel:2;
  } zhdr;
  static_assert(sizeof zhdr == 2, "!sizeof");

  memcpy(&zhdr, png->idat, 2);
  if (zhdr.cm != 8 || zhdr.cinfo > 7 || zhdr.fdict != 0 ||
      ((png->idat[0]<<8)+png->idat[1]) % 31 != 0)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

#define YF_NEXTBIT(x, pos) do { \
  const uint32_t b = (png->idat[off] & (1<<bit_off)) != 0; \
  x |= b << (pos); \
  if (++bit_off == 8) { \
    bit_off = 0; \
    ++off; \
  } } while (0);

  size_t off = 2, bit_off = 0;
  uint8_t bfinal, btype;

  do {
    bfinal = btype = 0;
    YF_NEXTBIT(bfinal, 0);
    YF_NEXTBIT(btype, 0);
    YF_NEXTBIT(btype, 1);

    if (btype == 0) {
      /* no compression */
      uint16_t len, nlen;
      if (bit_off != 0) {
        bit_off = 0;
        ++off;
      }
      memcpy(&len, &png->idat[off], sizeof len);
      off += sizeof len;
      memcpy(&nlen, &png->idat[off], sizeof nlen);
      off += sizeof nlen;
      if (len & nlen) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
      }
      len = be16toh(len);
      /* TODO */

    } else {
      if (btype == 1) {
        /* fixed H. codes */
        /* TODO */
      } else if (btype == 2) {
        /* dynamic H. codes */
        /* TODO */
      } else {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
      }
      uint32_t val = 0;
      do {
        /* TODO: Decode literal/length. */
        if (val < 256) {
          /* literal */
          /* TODO */
          break;
        } else if (val > 256) {
          /* reference */
          /* TODO: Decode distance. */
          /* TODO */
          break;
        } else {
          /* end of block */
          break;
        }
      } while (1);
    }
  } while (!bfinal);

  //////////
  printf("\n[%s]\n\n", __func__);

  printf("ihdr:\n wxh: %ux%u\n bd: %x\n ct: %x\n cm: %x\n fm: %x\n im: %x\n\n",
      png->ihdr->width, png->ihdr->height, png->ihdr->bit_depth,
      png->ihdr->color_type, png->ihdr->compression, png->ihdr->filter,
      png->ihdr->interlace);
  printf("plte (%p):\n size: %lu\n\n", (void *)png->plte, png->plte_sz);
  printf("idat (%p):\n size: %lu\n\n", (void *)png->idat, png->idat_sz);

  printf("zhdr:\n cm: %x\n cinfo: %x\n fcheck: %x\n fdict: %x\n flevel: %x\n\n",
      zhdr.cm, zhdr.cinfo, zhdr.fcheck, zhdr.fdict, zhdr.flevel);
  //////////

  /* TODO */
  return 0;
#undef YF_NEXTBIT
}

static int gen_codes(const uint8_t *lengths, size_t length_n, uint32_t *codes) {
  assert(lengths != NULL);
  assert(length_n > 0);
  assert(codes != NULL);

  /* max. bit length */
  size_t len_max = 0;
  for (size_t i = 0; i < length_n; ++i)
    len_max = YF_MAX(len_max, lengths[i]);

  if (len_max == 0) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  /* bit length count */
  uint8_t len_count[len_max+1];
  memset(len_count, 0, len_max+1);
  for (size_t i = 0; i < length_n; ++i)
    len_count[lengths[i]]++;

  /* initial codes */
  uint32_t code = 0;
  uint32_t next_code[len_max+1];
  memset(next_code, 0, (len_max+1)*sizeof next_code[0]);
  for (size_t bits = 1; bits <= len_max; ++bits) {
    code = (code+len_count[bits-1]) << 1;
    next_code[bits] = code;
  }

  /* code gen. */
  memset(codes, 0, length_n*sizeof *codes);
  for (size_t i = 0; i < length_n; i++) {
    uint8_t len = lengths[i];
    if (len != 0)
      codes[i] = next_code[len]++;
  }
  return 0;
}
