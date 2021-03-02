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

/* Code tree. */
typedef struct {
  int32_t leaf;
  union {
    uint16_t next[2];
    uint32_t value;
  };
} L_tree;

/* Generates Huffman codes from a sequence of code lengths and returns its
   tree representation. */
static L_tree *gen_codes(const uint8_t *lengths, size_t length_n,
    uint32_t *codes);

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
  const int r = load_texdt(&png, data);

  free(idat);
  free(plte);
  return r;
}

static int load_texdt(const L_png *png, YF_texdt *data) {
  assert(png != NULL);
  assert(data != NULL);

  /* image format */
  const uint32_t width = png->ihdr->width;
  const uint32_t height = png->ihdr->height;
  const uint8_t bit_depth = png->ihdr->bit_depth;
  int pixfmt;
  uint8_t channels;

  switch (png->ihdr->color_type) {
    case 0:
      /* greyscale */
      switch (png->ihdr->bit_depth) {
        case 1:
        case 2:
        case 4:
        case 8:
          pixfmt = YF_PIXFMT_R8UNORM;
          break;
        case 16:
          pixfmt = YF_PIXFMT_R16UNORM;
          break;
      }
      channels = 1;
      break;

    case 2:
      /* rgb */
      switch (png->ihdr->bit_depth) {
        case 8:
          pixfmt = YF_PIXFMT_RGB8SRGB;
          break;
        case 16:
          yf_seterr(YF_ERR_UNSUP, __func__);
          return -1;
        default:
          yf_seterr(YF_ERR_INVFILE, __func__);
          return -1;
      }
      channels = 3;
      break;

    case 3:
      /* palette indices */
      if (png->plte == NULL) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
      }
      switch (png->ihdr->bit_depth) {
        case 1:
        case 2:
        case 4:
        case 8:
          pixfmt = YF_PIXFMT_RGB8SRGB;
          break;
        default:
          yf_seterr(YF_ERR_INVFILE, __func__);
          return -1;
      }
      channels = 3;
      break;

    case 4:
      /* greyscale w/ alpha */
      switch (png->ihdr->bit_depth) {
        case 8:
          pixfmt = YF_PIXFMT_RG8UNORM;
          break;
        case 16:
          pixfmt = YF_PIXFMT_RG16UNORM;
          break;
        default:
          yf_seterr(YF_ERR_INVFILE, __func__);
          return -1;
      }
      channels = 2;
      break;

    case 6:
      /* rgba */
      switch (png->ihdr->bit_depth) {
        case 8:
          pixfmt = YF_PIXFMT_RGBA8SRGB;
          break;
        case 16:
          /* TODO */
          yf_seterr(YF_ERR_UNSUP, __func__);
          return -1;
        default:
          yf_seterr(YF_ERR_INVFILE, __func__);
          return -1;
      }
      channels = 4;
      break;
  }

  /* datastream */
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

  const size_t scln_len = 1+((width*channels*bit_depth+7)>>3);
  const size_t buf_len = scln_len*height;
  uint8_t *buf = malloc(buf_len);
  if (buf == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  size_t buf_off = 0;

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
        free(buf);
        return -1;
      }
      len = be16toh(len);
      if (len > buf_len) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(buf);
        return -1;
      }
      for (size_t i = 0; i < height; ++i) {
        memcpy(buf+buf_off, png->idat+off, scln_len);
        buf_off += scln_len;
        off += scln_len;
      }

    } else {
      struct { uint32_t codes[288]; L_tree *tree; } literal = {0};
      struct { uint32_t codes[32]; L_tree *tree; } distance = {0};

      if (btype == 1) {
        /* fixed H. codes */
        uint8_t lengths[288+32];
        memset(lengths, 8, 144);
        memset(lengths+144, 9, 112);
        memset(lengths+256, 7, 24);
        memset(lengths+280, 8, 8);
        memset(lengths+288, 5, 32);

        literal.tree = gen_codes(lengths, 288, literal.codes);
        distance.tree = gen_codes(lengths+288, 32, distance.codes);
        if (literal.tree == NULL || distance.tree == NULL) {
          free(literal.tree);
          free(distance.tree);
          free(buf);
          return -1;
        }

        //////////
        puts("#literal:");
        for (size_t i = 0; i < 288; ++i) {
          printf(" b");
          for (int j = lengths[i]-1; j >= 0; --j)
            printf("%d", literal.codes[i]>>j&1);
          printf(" (0x%x)\n", literal.codes[i]);
        }
        puts("#distance:");
        for (size_t i = 0; i < 32; ++i) {
          printf(" b");
          for (int j = lengths[i+288]-1; j >= 0; --j)
            printf("%d", distance.codes[i]>>j&1);
          printf(" (0x%x)\n", distance.codes[i]);
        }
        //////////

        /* TODO */

      } else if (btype == 2) {
        /* dynamic H. codes */
        uint8_t lengths[19+288+32] = {0};

        struct { uint16_t hlit:5, hdist:5, hclen:4; } bhdr = {0};
        for (size_t i = 0; i < 5; ++i)
          YF_NEXTBIT(bhdr.hlit, i);
        for (size_t i = 0; i < 5; ++i)
          YF_NEXTBIT(bhdr.hdist, i);
        for (size_t i = 0; i < 4; ++i)
          YF_NEXTBIT(bhdr.hclen, i);

        const uint8_t clen_map[] = {
          16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
        };
        for (size_t i = 0; i < bhdr.hclen+4U; ++i) {
          uint8_t len = 0;
          YF_NEXTBIT(len, 0);
          YF_NEXTBIT(len, 1);
          YF_NEXTBIT(len, 2);
          lengths[clen_map[i]] = len;
        }

        struct { uint32_t codes[19]; L_tree *tree; } clength;
        clength.tree = gen_codes(lengths, 19, clength.codes);
        if (clength.tree == NULL) {
          free(buf);
          return -1;
        }

        /* literal and distance lengths decoding */
        struct { int32_t len_n; uint32_t len_off; } ranges[2] = {
          {bhdr.hlit+257, 19},
          {bhdr.hdist+1, 19+288}
        };
        for (size_t i = 0; i < 2; ++i) {
          do {
            uint16_t idx = 0;
            do {
              uint8_t bit = 0;
              YF_NEXTBIT(bit, 0);
              idx = clength.tree[idx].next[bit];
            } while (!clength.tree[idx].leaf);
            uint32_t val = clength.tree[idx].value;
            if (val < 16) {
              lengths[ranges[i].len_off++] = val;
              --ranges[i].len_n;
            } else if (val == 16) {
              uint8_t rep = 0;
              YF_NEXTBIT(rep, 0);
              YF_NEXTBIT(rep, 1);
              rep += 3;
              memset(lengths+ranges[i].len_off, lengths[ranges[i].len_off-1],
                  rep*sizeof *lengths);
              ranges[i].len_off += rep;
              ranges[i].len_n -= rep;
            } else if (val == 17) {
              uint8_t rep = 0;
              YF_NEXTBIT(rep, 0);
              YF_NEXTBIT(rep, 1);
              YF_NEXTBIT(rep, 2);
              rep += 3;
              memset(lengths+ranges[i].len_off, 0, rep*sizeof *lengths);
              ranges[i].len_off += rep;
              ranges[i].len_n -= rep;
            } else if (val == 18) {
              uint8_t rep = 0;
              for (size_t i = 0; i < 7; ++i)
                YF_NEXTBIT(rep, i);
              rep += 11;
              memset(lengths+ranges[i].len_off, 0, rep*sizeof *lengths);
              ranges[i].len_off += rep;
              ranges[i].len_n -= rep;
            } else {
              assert(0);
            }
          } while (ranges[i].len_n > 0);
        }

        free(clength.tree);

        literal.tree = gen_codes(lengths+19, bhdr.hlit+257, literal.codes);
        distance.tree = gen_codes(lengths+19+288, bhdr.hdist+1, distance.codes);
        if (literal.tree == NULL || distance.tree == NULL) {
          free(literal.tree);
          free(distance.tree);
          free(buf);
          return -1;
        }

        //////////
        printf("hlit: %x\n", bhdr.hlit);
        printf("hdist: %x\n", bhdr.hdist);
        printf("hclen: %x\n", bhdr.hclen);
        for (size_t i = 0; i < sizeof lengths; ++i)
          printf(" #%lu: %u\n", i, lengths[i]);
        //////////

        /* TODO */

      } else {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(buf);
        return -1;
      }

      /* data decoding */
      do {
        uint16_t idx = 0;
        do {
          uint8_t bit = 0;
          YF_NEXTBIT(bit, 0);
          idx = literal.tree[idx].next[bit];
        } while (!literal.tree[idx].leaf);
        uint32_t val = literal.tree[idx].value;

        if (val < 256) {
          /* literal */
          buf[buf_off++] = val;

        } else if (val > 256) {
          /* <length, distance> pair */
          uint16_t len;
          if (val <= 264) {
            len = 10-(264-val);
          } else if (val <= 284) {
            const size_t v = val+4-265;
            const size_t bn = v>>2;
            const size_t rem = v&3;
            uint8_t ex = 0;
            for (size_t i = 0; i < bn; ++i)
              YF_NEXTBIT(ex, i);
            len = (1<<(bn+2))+(rem<<bn)+ex+3;
            assert(len < 258);
          } else {
            len = 258;
          }

          idx = 0;
          do {
            uint8_t bit = 0;
            YF_NEXTBIT(bit, 0);
            idx = distance.tree[idx].value;
          } while (!distance.tree[idx].leaf);
          val = distance.tree[idx].value;

          uint16_t dist;
          if (val <= 3) {
            dist = val+1;
          } else {
            const size_t bn = (val>>1)-1;
            uint16_t ex = 0;
            for (size_t i = 0; i < bn; ++i)
              YF_NEXTBIT(ex, i);
            dist = val&1 ? (3<<bn)+ex+1 : (2<<bn)+ex+1;
          }

          for (size_t i = 0; i < len; ++i) {
            buf[buf_off] = buf[buf_off-dist];
            ++buf_off;
          }

        } else {
          /* end of block */
          break;
        }
      } while (1);

      free(literal.tree);
      free(distance.tree);
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

static L_tree *gen_codes(const uint8_t *lengths, size_t length_n,
    uint32_t *codes)
{
  assert(lengths != NULL);
  assert(length_n > 0);
  assert(codes != NULL);

  /* max. bit length */
  size_t len_max = 0;
  for (size_t i = 0; i < length_n; ++i)
    len_max = YF_MAX(len_max, lengths[i]);

  if (len_max == 0) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return NULL;
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

  /* tree creation */
  size_t tree_n = 1;
  for (size_t i = 1; i <= len_max; ++i)
    tree_n += len_count[i]*i;
  L_tree *tree = calloc(tree_n, sizeof *tree);
  if (tree == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }

  size_t idx = 0;
  for (size_t i = 0; i < length_n; ++i) {
    if (lengths[i] == 0)
      continue;
    size_t cur = 0;
    for (int j = lengths[i]-1; j >= 0; --j) {
      uint8_t bit = codes[i]>>j&1;
      if (tree[cur].next[bit] == 0)
        tree[cur].next[bit] = ++idx;
      cur = tree[cur].next[bit];
    }
    tree[idx].leaf = 1;
    tree[idx].value = i;
  }

  if ((idx+1) < tree_n) {
    void *tmp = realloc(tree, (idx+1)*sizeof *tree);
    if (tmp != NULL)
      tree = tmp;
  }

  //////////
  printf("\n[tree] (idx: %lu, tree_n: %lu)\n", idx, tree_n);
  for (size_t i = 0; i <= idx; ++i) {
    if (tree[i].leaf)
      printf(" #%lu (value: %u)\n", i, tree[i].value);
    else
      printf(" #%lu (next: %u|%u)\n", i, tree[i].next[0], tree[i].next[1]);
  }
  printf("\n[codes]\n");
  for (size_t i = 0; i < length_n; ++i) {
    printf(" #%lu  b", i);
    for (int8_t j = lengths[i]-1; j >= 0; --j)
      printf("%d", codes[i]>>j&1);
    puts("");
  }
  //////////

  return tree;
}
