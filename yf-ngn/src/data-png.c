/*
 * YF
 * data-png.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef __STDC_NO_ATOMICS__
# include <stdatomic.h>
#else
# error "C11 atomics required"
#endif

#ifdef _DEFAULT_SOURCE
# include <endian.h>
#else
/* TODO */
# error "Invalid platform"
#endif

#ifdef YF_DEVEL
# include <stdio.h>
#endif

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"
#include "yf/core/yf-image.h"

#include "data-png.h"

#define YF_PNG_MAKETYPE(c1, c2, c3, c4) \
    ((c1 << 24) | (c2 << 16) | (c3 << 8) | c4)

/* Chunk. */
typedef struct {
    uint32_t len;
    uint32_t type;
    uint8_t var[];
} chunk_t;

#define YF_PNG_VARSZ 4096
static_assert(sizeof(chunk_t) == 8, "!sizeof");

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
} ihdr_t;

#define YF_PNG_IHDRSZ 13
static_assert(offsetof(ihdr_t, interlace) == YF_PNG_IHDRSZ-1, "!offsetof");

/* Palette. */
#define YF_PNG_PLTE YF_PNG_MAKETYPE('P', 'L', 'T', 'E')

/* Image data. */
#define YF_PNG_IDAT YF_PNG_MAKETYPE('I', 'D', 'A', 'T')

/* Image trailer. */
#define YF_PNG_IEND YF_PNG_MAKETYPE('I', 'E', 'N', 'D')

/* PNG file signature. */
static const uint8_t sign_[8] = {137, 80, 78, 71, 13, 10, 26, 10};

/* CRC table. */
static uint32_t crctab_[256] = {0};
static atomic_int crcsync_ = 0;

#define YF_PNG_INITCRC() do { \
    int expect = 0; \
    if (atomic_compare_exchange_strong(&crcsync_, &expect, -1)) { \
        for (uint32_t i = 0; i < 256; i++) { \
            uint32_t crc = i, j = 8; \
            while (j--) \
                crc = (crc & 1) ? (0xedb88320 ^ (crc >> 1)) : (crc >> 1); \
            crctab_[i] = crc; \
        } \
        atomic_store(&crcsync_, 1); \
    } else { \
        while (atomic_load(&crcsync_) != 1) \
            ; \
    } } while (0)

#define YF_PNG_CALCCRC(crc, data, len) do { \
    crc = ~0U; \
    for (size_t i = 0; i < (len); i++) \
        crc = crctab_[((crc) ^ (data)[i]) & 0xff] ^ ((crc) >> 8); \
    crc ^= ~0U; } while (0)

/* Processed PNG chunks. */
typedef struct {
    ihdr_t *ihdr;
    uint8_t *plte;
    size_t plte_sz;
    uint8_t *idat;
    size_t idat_sz;
} png_t;

/* Code tree. */
typedef struct {
    int32_t leaf;
    union {
        uint16_t next[2];
        uint32_t value;
    };
} tree_t;

#if defined(YF_DEVEL) && defined(YF_PRINT)
static void print_codes(const uint8_t *lengths, size_t length_n,
                        const uint32_t *codes, const tree_t *tree,
                        size_t tree_n, size_t tree_max);
#endif

/* Generates Huffman codes from a sequence of code lengths and returns its
   tree representation. */
static tree_t *gen_codes(const uint8_t *lengths, size_t length_n,
                         uint32_t *codes)
{
    assert(lengths != NULL);
    assert(length_n > 0);
    assert(codes != NULL);

    /* max. bit length */
    size_t len_max = 0;
    for (size_t i = 0; i < length_n; i++)
        len_max = YF_MAX(len_max, lengths[i]);

    if (len_max == 0) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    /* bit length count */
    uint8_t len_count[len_max+1];
    memset(len_count, 0, len_max+1);
    for (size_t i = 0; i < length_n; i++)
        len_count[lengths[i]]++;

    /* initial codes */
    uint32_t code = 0;
    uint32_t next_code[len_max+1];
    memset(next_code, 0, (len_max+1)*sizeof next_code[0]);
    for (size_t bits = 1; bits <= len_max; bits++) {
        code = (code+len_count[bits-1]) << 1;
        next_code[bits] = code;
    }

    /* code gen. */
    memset(codes, 0, length_n*sizeof *codes);
    size_t code_n = 0;
    for (size_t i = 0; i < length_n; i++) {
        if (lengths[i] != 0) {
            codes[i] = next_code[lengths[i]]++;
            code_n++;
        }
    }

    /* tree creation */
    size_t tree_n = code_n << 1;
    tree_t *tree = calloc(tree_n, sizeof *tree);
    if (tree == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    size_t indx = 0;
    for (size_t i = 0; i < length_n; i++) {
        if (lengths[i] == 0)
            continue;
        size_t cur = 0;
        for (int j = lengths[i]-1; j >= 0; j--) {
            uint8_t bit = codes[i]>>j&1;
            if (tree[cur].next[bit] == 0)
                tree[cur].next[bit] = ++indx;
            cur = tree[cur].next[bit];
        }
        tree[indx].leaf = 1;
        tree[indx].value = i;
    }

#if defined(YF_DEVEL) && defined(YF_PRINT)
    print_codes(lengths, length_n, codes, tree, indx+1, tree_n);
#endif

    return tree;
}

/* Decompresses a datastream.
   The caller must ensure that 'buf' is large enough to store the entirety of
   decompressed data. */
static int inflate(const uint8_t *strm, uint8_t *buf, size_t buf_sz)
{
    assert(strm != NULL);
    assert(buf != NULL);

    struct {
        uint8_t cm:4, cinfo:4;
        uint8_t fcheck:5, fdict:1, flevel:2;
    } zhdr;
    static_assert(sizeof zhdr == 2, "!sizeof");

    memcpy(&zhdr, strm, 2);
    if (zhdr.cm != 8 || zhdr.cinfo > 7 || zhdr.fdict != 0 ||
        ((strm[0]<<8)+strm[1]) % 31 != 0) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

#define YF_NEXTBIT(x, pos) do { \
    const uint32_t b = (strm[off] & (1<<bit_off)) != 0; \
    x |= b << (pos); \
    if (++bit_off == 8) { \
        bit_off = 0; \
        off++; \
    } } while (0);

    size_t off = 2, bit_off = 0, buf_off = 0;
    uint8_t bfinal, btype;

    do {
        bfinal = 0;
        YF_NEXTBIT(bfinal, 0);

        btype = 0;
        YF_NEXTBIT(btype, 0);
        YF_NEXTBIT(btype, 1);

        if (btype == 0) {
            /* uncompressed */
            uint16_t len, nlen;
            if (bit_off != 0) {
                bit_off = 0;
                off++;
            }

            memcpy(&len, strm+off, sizeof len);
            off += sizeof len;
            memcpy(&nlen, strm+off, sizeof nlen);
            off += sizeof nlen;
            if (len & nlen) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                return -1;
            }

            len = le16toh(len);
            if (len+buf_off > buf_sz) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                return -1;
            }

            memcpy(buf+buf_off, strm+off, len);
            off += len;
            buf_off += len;

        } else {
            /* compressed */
            struct { uint32_t codes[288]; tree_t *tree; } literal = {0};
            struct { uint32_t codes[32]; tree_t *tree; } distance = {0};

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
                    return -1;
                }

            } else if (btype == 2) {
                /* dynamic H. codes */
                uint8_t lengths[19+288+32] = {0};

                struct { uint16_t hlit:5, hdist:5, hclen:4; } bhdr = {0};
                for (size_t i = 0; i < 5; i++)
                    YF_NEXTBIT(bhdr.hlit, i);
                for (size_t i = 0; i < 5; i++)
                    YF_NEXTBIT(bhdr.hdist, i);
                for (size_t i = 0; i < 4; i++)
                    YF_NEXTBIT(bhdr.hclen, i);

                const uint8_t clen_map[] = {
                    16, 17, 18, 0, 8, 7, 9, 6, 10, 5,
                    11, 4, 12, 3, 13, 2, 14, 1, 15
                };
                for (size_t i = 0; i < bhdr.hclen+4U; i++) {
                    uint8_t len = 0;
                    YF_NEXTBIT(len, 0);
                    YF_NEXTBIT(len, 1);
                    YF_NEXTBIT(len, 2);
                    lengths[clen_map[i]] = len;
                }

                struct { uint32_t codes[19]; tree_t *tree; } clength;
                clength.tree = gen_codes(lengths, 19, clength.codes);
                if (clength.tree == NULL)
                    return -1;

                /* literal and distance lengths decoding */
                struct { int32_t len_n; uint32_t len_off; } ranges[2] = {
                    {bhdr.hlit+257, 19},
                    {bhdr.hdist+1, 19+288}
                };

                for (size_t i = 0; i < 2; i++) {
                    do {
                        uint16_t indx = 0;
                        do {
                            uint8_t bit = 0;
                            YF_NEXTBIT(bit, 0);
                            indx = clength.tree[indx].next[bit];
                        } while (!clength.tree[indx].leaf);
                        uint32_t val = clength.tree[indx].value;

                        if (val < 16) {
                            /* code length */
                            lengths[ranges[i].len_off++] = val;
                            ranges[i].len_n--;

                        } else if (val == 16) {
                            /* copy prev. */
                            uint8_t rep = 0;
                            YF_NEXTBIT(rep, 0);
                            YF_NEXTBIT(rep, 1);
                            rep += 3;
                            memset(lengths+ranges[i].len_off,
                                   lengths[ranges[i].len_off-1],
                                   rep*sizeof *lengths);
                            ranges[i].len_off += rep;
                            ranges[i].len_n -= rep;

                        } else if (val == 17) {
                            /* repeat (3-bit) */
                            uint8_t rep = 0;
                            YF_NEXTBIT(rep, 0);
                            YF_NEXTBIT(rep, 1);
                            YF_NEXTBIT(rep, 2);
                            rep += 3;
                            memset(lengths+ranges[i].len_off, 0,
                                   rep*sizeof *lengths);
                            ranges[i].len_off += rep;
                            ranges[i].len_n -= rep;

                        } else if (val == 18) {
                            /* repeat (7-bit) */
                            uint8_t rep = 0;
                            for (size_t i = 0; i < 7; i++)
                                YF_NEXTBIT(rep, i);
                            rep += 11;
                            memset(lengths+ranges[i].len_off, 0,
                                   rep*sizeof *lengths);
                            ranges[i].len_off += rep;
                            ranges[i].len_n -= rep;

                        } else {
                            assert(0);
                        }
                    } while (ranges[i].len_n > 0);
                }

                free(clength.tree);

                literal.tree = gen_codes(lengths+19, bhdr.hlit+257,
                                         literal.codes);
                distance.tree = gen_codes(lengths+19+288, bhdr.hdist+1,
                                          distance.codes);
                if (literal.tree == NULL || distance.tree == NULL) {
                    free(literal.tree);
                    free(distance.tree);
                    return -1;
                }

            } else {
                yf_seterr(YF_ERR_INVFILE, __func__);
                return -1;
            }

            /* data decoding */
            while (1) {
                uint16_t indx = 0;
                do {
                    uint8_t bit = 0;
                    YF_NEXTBIT(bit, 0);
                    indx = literal.tree[indx].next[bit];
                } while (!literal.tree[indx].leaf);
                uint32_t val = literal.tree[indx].value;

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
                        for (size_t i = 0; i < bn; i++)
                            YF_NEXTBIT(ex, i);
                        len = (1<<(bn+2))+(rem<<bn)+ex+3;
                        assert(len < 258);
                    } else {
                        len = 258;
                    }

                    indx = 0;
                    do {
                        uint8_t bit = 0;
                        YF_NEXTBIT(bit, 0);
                        indx = distance.tree[indx].next[bit];
                    } while (!distance.tree[indx].leaf);
                    val = distance.tree[indx].value;

                    uint16_t dist;
                    if (val <= 3) {
                        dist = val+1;
                    } else {
                        const size_t bn = (val>>1)-1;
                        uint16_t ex = 0;
                        for (size_t i = 0; i < bn; i++)
                            YF_NEXTBIT(ex, i);
                        dist = val&1 ? (3<<bn)+ex+1 : (2<<bn)+ex+1;
                    }

                    for (size_t i = 0; i < len; i++) {
                        buf[buf_off] = buf[buf_off-dist];
                        buf_off++;
                    }

                } else {
                    /* end of block */
                    break;
                }
            }

            free(literal.tree);
            free(distance.tree);
        }
    } while (!bfinal);

#undef YF_NEXTBIT
    return 0;
}

/* Loads texture data from processed chunks. */
static int load_texdt(const png_t *png, yf_texdt_t *data)
{
    assert(png != NULL);
    assert(data != NULL);

    /* image format */
    const uint32_t width = png->ihdr->width;
    const uint32_t height = png->ihdr->height;
    const uint8_t bit_depth = png->ihdr->bit_depth;
    int pixfmt;
    uint8_t channels;

    /* TODO: Check format support. */
    /* TODO: Choose sRGB formats whenever possible. */
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
            pixfmt = YF_PIXFMT_RGB8UNORM;
            break;
        case 16:
            pixfmt = YF_PIXFMT_RGB16UNORM;
            break;
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
            pixfmt = YF_PIXFMT_RGB8UNORM;
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
            pixfmt = YF_PIXFMT_RGBA8UNORM;
            break;
        case 16:
            pixfmt = YF_PIXFMT_RGBA16UNORM;
            break;
        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
        channels = 4;
        break;
    }

    /* decompression */
    const size_t scln_sz = png->ihdr->color_type & 1 ?
                           1+width :
                           1+((width*channels*bit_depth+7)>>3);
    const size_t buf_sz = scln_sz*height;

    uint8_t *buf = malloc(buf_sz);
    if (buf == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    if (inflate(png->idat, buf, buf_sz) != 0) {
        free(buf);
        return -1;
    }

    /* filter reversion */
    const uint8_t bypp = YF_MAX(1, (channels*bit_depth)>>3);

    switch (buf[0]) {
    case 0:
    case 2:
        /* none/up */
        break;
    case 1:
    case 4:
        /* sub/paeth */
        for (size_t i = 1+bypp; i < scln_sz; i++)
            buf[i] += buf[i-bypp];
        break;
    case 3:
        /* avg */
        for (size_t i = 1+bypp; i < scln_sz; i++)
            buf[i] += buf[i-bypp]>>1;
        break;
    default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(buf);
        return -1;
    }

    for (size_t i = 1; i < height; i++) {
        switch (buf[i*scln_sz]) {
        case 0:
            /* none */
            break;
        case 1:
            /* sub */
            for (size_t j = 1+bypp; j < scln_sz; j++) {
                const size_t k = i*scln_sz+j;
                buf[k] += buf[k-bypp];
            }
            break;
        case 2:
            /* up */
            for (size_t j = 1; j < scln_sz; j++) {
                const size_t k = i*scln_sz+j;
                buf[k] += buf[k-scln_sz];
            }
            break;
        case 3:
            /* avg */
            for (size_t j = 1; j <= bypp; j++) {
                const size_t k = i*scln_sz+j;
                buf[k] += buf[k-scln_sz]>>1;
            }
            for (size_t j = 1+bypp; j < scln_sz; j++) {
                const size_t k = i*scln_sz+j;
                const uint16_t a = buf[k-bypp];
                const uint16_t b = buf[k-scln_sz];
                buf[k] += (a+b)>>1;
            }
            break;
        case 4:
            /* paeth */
            for (size_t j = 1; j <= bypp; j++) {
                const size_t k = i*scln_sz+j;
                buf[k] += buf[k-scln_sz];
            }
            for (size_t j = 1+bypp; j < scln_sz; j++) {
                const size_t k = i*scln_sz+j;
                const int16_t a = buf[k-bypp];
                const int16_t b = buf[k-scln_sz];
                const int16_t c = buf[k-scln_sz-bypp];
                const int16_t p = a+b-c;
                const uint16_t pa = abs(p-a);
                const uint16_t pb = abs(p-b);
                const uint16_t pc = abs(p-c);
                buf[k] += pa<=pb && pa<=pc ? a : (pb<=pc ? b : c);
            }
            break;
        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            free(buf);
            return -1;
        }
    }

    /* texture data */
    if (png->ihdr->color_type & 1) {
        if (bit_depth == 8) {
            /* palette indices 8 bit depth */
            const size_t new_sz = (buf_sz-height)*3;
            void *tmp = realloc(buf, new_sz);
            if (tmp == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                free(buf);
                return -1;
            }
            buf = tmp;
            size_t rgb_off = 0;
            size_t indx_off = new_sz-buf_sz;
            memmove(buf+indx_off, buf, buf_sz);
            for (size_t i = 0; i < height; i++) {
                indx_off++;
                for (size_t j = 0; j < scln_sz-1; j++) {
                    const uint8_t indx = buf[indx_off++];
                    memcpy(buf+rgb_off, png->plte+indx, 3);
                    rgb_off += 3;
                }
            }
        } else {
            /* palette indices 1/2/4 bit depth */
            const size_t new_sz = width*height*3;
            void *tmp = realloc(buf, new_sz);
            if (tmp == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                free(buf);
                return -1;
            }
            buf = tmp;
            size_t rgb_off = 0;
            size_t indx_off = new_sz-buf_sz;
            size_t bit_off = 0;
            memmove(buf+indx_off, buf, buf_sz);
            for (size_t i = 0; i < height; i++) {
                indx_off++;
                for (size_t j = 0; j < width; j++) {
                    const uint8_t indx = buf[indx_off] >>
                                         (8-bit_depth-bit_off) &
                                         ((1<<bit_depth)-1);
                    memcpy(buf+rgb_off, png->plte+indx, 3);
                    rgb_off += 3;
                    const div_t d = div(bit_off+bit_depth, 8);
                    indx_off += d.quot;
                    bit_off = d.rem;
                }
                if (bit_off != 0) {
                    bit_off = 0;
                    indx_off++;
                }
            }
        }

    } else if (bit_depth >= 8) {
        /* rgb[a]/greyscale[a] 8/16 bit depth */
        for (size_t i = 0; i < height; i++) {
            const size_t j = i*scln_sz-i;
            memmove(buf+j, buf+j+i+1, scln_sz-1);
        }
        void *tmp = realloc(buf, buf_sz-height);
        if (tmp != NULL)
            buf = tmp;
        if (bit_depth == 16) {
            for (size_t i = 0; i < buf_sz-height; i += 2) {
                const uint16_t val = be16toh((buf[i+1]<<8)|buf[i]);
                memcpy(buf+i, &val, sizeof val);
            }
        }

    } else {
        /* greyscale 1/2/4 bit depth */
        uint8_t *tmp = malloc(width*height);
        if (tmp == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            free(buf);
            return -1;
        }
        size_t off = 0, bit_off = 0;
        for (size_t i = 0; i < height; i++) {
            off++;
            for (size_t j = 0; j < width; j++) {
                tmp[i*width+j] = buf[off] >> (8-bit_depth-bit_off) &
                    ((1<<bit_depth)-1);
                const div_t d = div(bit_off+bit_depth, 8);
                off += d.quot;
                bit_off = d.rem;
            }
            if (bit_off != 0) {
                bit_off = 0;
                off++;
            }
        }
        free(buf);
        buf = tmp;
    }

    data->data = buf;
    data->pixfmt = pixfmt;
    data->dim.width = width;
    data->dim.height = height;
    /* XXX: Default sampler params. and 'UVSET' value. */
    data->splr = (yf_sampler_t){0};
    data->uvset = YF_UVSET_0;

    return 0;
}

static int load_png(FILE *file, yf_texdt_t *data)
{
    assert(file != NULL && !feof(file));
    assert(data != NULL);

    uint8_t sign[sizeof sign_];
    if (fread(sign, sizeof sign, 1, file) < 1 ||
        memcmp(sign_, sign, sizeof sign) != 0) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    chunk_t *chunk = malloc(sizeof(chunk_t)+YF_PNG_VARSZ);
    if (chunk == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    size_t var_sz = YF_PNG_VARSZ;
    uint32_t len, type, crc, crc_res;

    YF_PNG_INITCRC();

    /* IHDR */
    if (fread(chunk, sizeof(chunk_t), 1, file) < 1) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(chunk);
        return -1;
    }
    len = be32toh(chunk->len);
    type = be32toh(chunk->type);

    static_assert(YF_PNG_IHDRSZ <= YF_PNG_VARSZ);

    if (type != YF_PNG_IHDR || len != YF_PNG_IHDRSZ ||
        fread(chunk->var, len, 1, file) < 1) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(chunk);
        return -1;
    }

    if (fread(&crc, sizeof crc, 1, file) < 1) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(chunk);
        return -1;
    }
    crc = be32toh(crc);
    YF_PNG_CALCCRC(crc_res, (uint8_t *)(&chunk->type), len+sizeof chunk->type);
    if (crc != crc_res) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(chunk);
        return -1;
    }

    ihdr_t ihdr;
    memcpy(&ihdr, chunk->var, len);
    ihdr.width = be32toh(ihdr.width);
    ihdr.height = be32toh(ihdr.height);

    if (ihdr.width == 0 || ihdr.height == 0 ||
        (ihdr.bit_depth != 1 && ihdr.bit_depth != 2 && ihdr.bit_depth != 4 &&
         ihdr.bit_depth != 8 && ihdr.bit_depth != 16) ||
        (ihdr.color_type != 0 && ihdr.color_type != 2 && ihdr.color_type != 3 &&
         ihdr.color_type != 4 && ihdr.color_type != 6) ||
        ihdr.compression != 0 || ihdr.filter != 0 || ihdr.interlace > 1) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(chunk);
        return -1;
    }

    /* TODO: Decoding of interlaced images. */
    if (ihdr.interlace != 0) {
        yf_seterr(YF_ERR_UNSUP, __func__);
        free(chunk);
        return -1;
    }

    /* chunk processing */
    uint8_t *plte = NULL;
    size_t plte_sz = 0;
    uint8_t *idat = NULL;
    size_t idat_sz = 0;

    while (1) {
        if (fread(chunk, sizeof(chunk_t), 1, file) < 1) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            free(chunk);
            return -1;
        }
        const int critical = !(chunk->type & 0x20);
        len = be32toh(chunk->len);
        type = be32toh(chunk->type);

        if (len > var_sz) {
            const size_t new_sz = sizeof(chunk_t)+len;
            void *tmp = realloc(chunk, new_sz);
            if (tmp != NULL) {
                chunk = tmp;
                var_sz = new_sz;
            } else if (critical) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                free(idat);
                free(plte);
                free(chunk);
                return -1;
            } else if (fseek(file, len+sizeof crc, SEEK_CUR) != 0) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                free(idat);
                free(plte);
                free(chunk);
                return -1;
            } else {
                continue;
            }
        }

        if (type == YF_PNG_IEND) {
            /* IEND */
            break;

        } else if (len == 0) {
            /* no data */

        } else if (type == YF_PNG_PLTE) {
            /* PLTE */
            if (ihdr.color_type == 0 || ihdr.color_type == 4 || len % 3 != 0 ||
                plte != NULL || idat != NULL) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                free(idat);
                free(plte);
                free(chunk);
                return -1;
            }
            if (fread(chunk->var, len, 1, file) < 1) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                free(plte);
                free(chunk);
                return -1;
            }
            plte = malloc(len);
            if (plte == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                return -1;
            }
            memcpy(plte, chunk->var, len);
            plte_sz = len;

        } else if (type == YF_PNG_IDAT) {
            /* IDAT */
            if (fread(chunk->var, len, 1, file) < 1) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                free(idat);
                free(plte);
                free(chunk);
                return -1;
            }
            void *tmp = realloc(idat, idat_sz+len);
            if (tmp == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                free(idat);
                free(plte);
                free(chunk);
                return -1;
            }
            idat = tmp;
            memcpy(idat+idat_sz, chunk->var, len);
            idat_sz += len;

        } else if (critical) {
            /* unsupported */
            yf_seterr(YF_ERR_UNSUP, __func__);
            free(idat);
            free(plte);
            free(chunk);
            return -1;

        } else {
            /* unknown */
            if (fseek(file, len+sizeof crc, SEEK_CUR) != 0) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                free(idat);
                free(plte);
                free(chunk);
                return -1;
            }
            continue;
        }

        if (fread(&crc, sizeof crc, 1, file) < 1) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            free(idat);
            free(plte);
            free(chunk);
            return -1;
        }
        crc = be32toh(crc);
        YF_PNG_CALCCRC(crc_res, (uint8_t *)(&chunk->type),
                       len+sizeof chunk->type);
        if (crc != crc_res) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            free(idat);
            free(plte);
            free(chunk);
            return -1;
        }
    }

    free(chunk);

    if (idat == NULL) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(plte);
        return -1;
    }

    /* data processing */
    const png_t png = {
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

int yf_loadpng(const char *pathname, yf_texdt_t *data)
{
    assert(pathname != NULL);
    assert(data != NULL);

    FILE *file = fopen(pathname, "r");
    if (file == NULL) {
        yf_seterr(YF_ERR_NOFILE, __func__);
        return -1;
    }

    int r = load_png(file, data);
    fclose(file);
    return r;
}

int yf_loadpng2(FILE *file, yf_texdt_t *data)
{
    assert(file != NULL && !feof(file));
    assert(data != NULL);

    return load_png(file, data);
}

/*
 * DEVEL
 */

#if defined(YF_DEVEL) && defined(YF_PRINT)

static void print_codes(const uint8_t *lengths, size_t length_n,
                        const uint32_t *codes, const tree_t *tree,
                        size_t tree_n, size_t tree_max)
{
    printf("\n[YF] OUTPUT (%s):\n", __func__);

    printf(" codes (%zu):\n", length_n);
    for (size_t i = 0; i < length_n; i++) {
        printf("  [%.4zu]  b", i);
        for (int8_t j = lengths[i]-1; j >= 0; j--)
            printf("%d", codes[i]>>j&1);
        puts("");
    }

    printf(" code tree (%zu, max=%zu):\n", tree_n, tree_max);
    for (size_t i = 0; i < tree_n; i++) {
        if (tree[i].leaf)
            printf("  [%.4zu]  value: %u\n", i, tree[i].value);
        else
            printf("  [%.4zu]  next: %u|%u\n", i, tree[i].next[0],
                   tree[i].next[1]);
    }

    puts("");
}

#endif
