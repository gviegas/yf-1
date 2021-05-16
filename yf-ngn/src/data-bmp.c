/*
 * YF
 * data-bmp.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _DEFAULT_SOURCE
# include <endian.h>
#else
/* TODO */
# error "Invalid platform"
#endif

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"
#include "yf/core/yf-image.h"

#include "data-bmp.h"

/* Type representing the BMP file header. */
typedef struct {
    uint8_t _[2];
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t data_off;
} T_bmpfh;
#define YF_BMPFH_SZ  14
static_assert(offsetof(T_bmpfh, data_off) == YF_BMPFH_SZ-4+2, "!offsetof");

/* Type representing the BMP info header. */
typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t img_sz;
    int32_t ppm_x;
    int32_t ppm_y;
    uint32_t ci_n;
    uint32_t ci_important;
} T_bmpih;
#define YF_BMPIH_SZ  40
static_assert(offsetof(T_bmpih, ci_important) == YF_BMPIH_SZ-4, "!offsetof");

/* Type representing the BMP version 4 header. */
typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t img_sz;
    int32_t ppm_x;
    int32_t ppm_y;
    uint32_t ci_n;
    uint32_t ci_important;
    uint32_t mask_r;
    uint32_t mask_g;
    uint32_t mask_b;
    uint32_t mask_a;
    uint32_t cs_type;
    uint32_t end_pts[9];
    uint32_t gamma_r;
    uint32_t gamma_g;
    uint32_t gamma_b;
} T_bmpv4h;
#define YF_BMPV4H_SZ 108
static_assert(offsetof(T_bmpv4h, gamma_b) == YF_BMPV4H_SZ-4, "!offsetof");

/* Type representing the BMP version 5 header. */
typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t img_sz;
    int32_t ppm_x;
    int32_t ppm_y;
    uint32_t ci_n;
    uint32_t ci_important;
    uint32_t mask_r;
    uint32_t mask_g;
    uint32_t mask_b;
    uint32_t mask_a;
    uint32_t cs_type;
    uint32_t end_pts[9];
    uint32_t gamma_r;
    uint32_t gamma_g;
    uint32_t gamma_b;
    uint32_t intent;
    uint32_t prof_dt;
    uint32_t prof_sz;
    uint32_t reserved;
} T_bmpv5h;
#define YF_BMPV5H_SZ 124
static_assert(offsetof(T_bmpv5h, reserved) == YF_BMPV5H_SZ-4, "!offsetof");

/* BMP format definitions. */
#define YF_BMP_TYPE         0x4d42
#define YF_BMP_COMPR_RGB    0
#define YF_BMP_COMPR_BITFLD 3

/* Counts the number of unset low bits in a mask. */
#define YF_SETLSHF(res, mask, bpp) do { \
    res = 0; \
    while (res != bpp && (mask & (1 << res)) == 0) ++res; } while (0)

/* Counts the number of bits set in a mask. */
#define YF_SETBITN(res, mask, bpp, lshf) do { \
    res = bpp; \
    while (res > lshf && (mask & (1 << (res-1))) == 0) --res; \
    res -= lshf; } while (0)

int yf_loadbmp(const char *pathname, YF_texdt *data)
{
    assert(data != NULL);

    if (pathname == NULL) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    FILE *file = fopen(pathname, "r");
    if (file == NULL) {
        yf_seterr(YF_ERR_NOFILE, __func__);
        return -1;
    }

    T_bmpfh fh;
    if (fread(&fh.type, 1, YF_BMPFH_SZ, file) < YF_BMPFH_SZ ||
        le16toh(fh.type) != YF_BMP_TYPE) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        fclose(file);
        return -1;
    }
    uint32_t data_off = le32toh(fh.data_off);
    uint32_t hdr_sz;
    if (fread(&hdr_sz, sizeof hdr_sz, 1, file) < 1) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        fclose(file);
        return -1;
    }
    hdr_sz = le32toh(hdr_sz);

    uint32_t rd_n = YF_BMPFH_SZ + sizeof hdr_sz;
    int32_t w;
    int32_t h;
    uint16_t bpp;
    uint32_t compr;
    uint32_t ci_n;
    uint32_t mask_rgba[4];
    uint32_t lshf_rgba[4];
    uint32_t bitn_rgba[4];

    switch (hdr_sz) {
    case YF_BMPIH_SZ: {
        T_bmpih ih;
        ih.size = htole32(hdr_sz);
        uint32_t n = YF_BMPIH_SZ - sizeof hdr_sz;
        if (fread(&ih.width, 1, n, file) < n) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            fclose(file);
            return -1;
        }
        rd_n += n;
        w = le32toh(ih.width);
        h = le32toh(ih.height);
        bpp = le16toh(ih.bpp);
        compr = le32toh(ih.compression);
        ci_n = le32toh(ih.ci_n);
        /* the info header supports non-alpha colors only */
        mask_rgba[3] = 0;
        lshf_rgba[3] = bpp;
        bitn_rgba[3] = 0;
        switch (compr) {
        case YF_BMP_COMPR_RGB:
            break;
        case YF_BMP_COMPR_BITFLD:
            if (rd_n == data_off ||
                fread(mask_rgba, sizeof *mask_rgba, 3, file) < 3) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                fclose(file);
                return -1;
            }
            mask_rgba[0] = le32toh(mask_rgba[0]);
            mask_rgba[1] = le32toh(mask_rgba[1]);
            mask_rgba[2] = le32toh(mask_rgba[2]);
            rd_n += 3 * sizeof *mask_rgba;
            break;
        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            fclose(file);
            return -1;
        }
    } break;

    case YF_BMPV4H_SZ: {
        T_bmpv4h v4h;
        v4h.size = htole32(hdr_sz);
        uint32_t n = YF_BMPV4H_SZ - sizeof hdr_sz;
        if (fread(&v4h.width, 1, n, file) < n) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            fclose(file);
            return -1;
        }
        rd_n += n;
        w = le32toh(v4h.width);
        h = le32toh(v4h.height);
        bpp = le16toh(v4h.bpp);
        compr = le32toh(v4h.compression);
        ci_n = le32toh(v4h.ci_n);
        mask_rgba[3] = (bpp == 16 || bpp == 32) ? le32toh(v4h.mask_a) : 0;
        switch (compr) {
        case YF_BMP_COMPR_RGB:
            break;
        case YF_BMP_COMPR_BITFLD:
            mask_rgba[0] = le32toh(v4h.mask_r);
            mask_rgba[1] = le32toh(v4h.mask_g);
            mask_rgba[2] = le32toh(v4h.mask_b);
            break;
        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            fclose(file);
            return -1;
        }
    } break;

    case YF_BMPV5H_SZ: {
        T_bmpv5h v5h;
        v5h.size = htole32(hdr_sz);
        uint32_t n = YF_BMPV5H_SZ - sizeof hdr_sz;
        if (fread(&v5h.width, 1, n, file) < n) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            fclose(file);
            return -1;
        }
        rd_n += n;
        w = le32toh(v5h.width);
        h = le32toh(v5h.height);
        bpp = le16toh(v5h.bpp);
        compr = le32toh(v5h.compression);
        ci_n = le32toh(v5h.ci_n);
        mask_rgba[3] = (bpp == 16 || bpp == 32) ? le32toh(v5h.mask_a) : 0;
        switch (compr) {
        case YF_BMP_COMPR_RGB:
            break;
        case YF_BMP_COMPR_BITFLD:
            mask_rgba[0] = le32toh(v5h.mask_r);
            mask_rgba[1] = le32toh(v5h.mask_g);
            mask_rgba[2] = le32toh(v5h.mask_b);
            break;
        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            fclose(file);
            return -1;
        }
    } break;

    default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        fclose(file);
        return -1;
    }

    if (w <= 0 || h == 0) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        fclose(file);
        return -1;
    }
    if (bpp > 8 && rd_n != data_off && fseek(file, data_off, SEEK_SET) != 0) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        fclose(file);
        return -1;
    }

    size_t channels = mask_rgba[3] != 0 ? 4 : 3;
    unsigned char *dt = malloc(channels * w * (h < 0 ? -h : h));
    if (dt == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        fclose(file);
        return -1;
    }
    unsigned char *scln = NULL;

    /* pixel data can be either bottom-up or top-down */
    int32_t from, to, inc;
#ifdef YF_FLIP_TEX
    if (h > -1) {
        from = 0;
        to = h;
        inc = 1;
    } else {
        from = -h-1;
        to = -1;
        inc = -1;
    }
#else
    if (h > -1) {
        from = h-1;
        to = -1;
        inc = -1;
    } else {
        from = 0;
        to = -h;
        inc = 1;
    }
#endif

    switch (bpp) {
    case 8: {
        if (ci_n == 0)
            ci_n = 256;
        uint32_t ci[ci_n];
        if (fread(ci, sizeof *ci, ci_n, file) < ci_n) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            fclose(file);
            free(dt);
            return -1;
        }
        size_t padding = w & 3 ? 4 - (w & 3) : 0;
        size_t scln_sz = w + padding;
        if ((scln = malloc(scln_sz)) == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            fclose(file);
            free(dt);
            return -1;
        }
        for (int32_t i = from; i != to; i += inc) {
            if (fread(scln, 1, scln_sz, file) < scln_sz) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                fclose(file);
                free(dt);
                free(scln);
                return -1;
            }
            for (int32_t j = 0; j < w; ++j) {
                size_t dt_i = channels*w*i + channels*j;
                size_t ci_i = scln[j];
                memcpy(dt+dt_i, ci+ci_i, channels);
            }
        }
    } break;

    case 16: {
        if (compr == YF_BMP_COMPR_RGB) {
            mask_rgba[0] = 0x7c00;
            mask_rgba[1] = 0x03e0;
            mask_rgba[2] = 0x001f;
        }
        for (size_t i = 0; i < channels; ++i) {
            YF_SETLSHF(lshf_rgba[i], mask_rgba[i], bpp);
            YF_SETBITN(bitn_rgba[i], mask_rgba[i], bpp, lshf_rgba[i]);
        }
        size_t padding = (w & 1) << 1;
        size_t scln_sz = (w << 1) + padding;
        if ((scln = malloc(scln_sz)) == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            fclose(file);
            free(dt);
            return -1;
        }
        /* each channel will be scaled to the 8-bit range */
        uint32_t diff_rgba[4] = {
            YF_MIN(8, 8-bitn_rgba[0]),
            YF_MIN(8, 8-bitn_rgba[1]),
            YF_MIN(8, 8-bitn_rgba[2]),
            YF_MIN(8, 8-bitn_rgba[3])
        };
        for (int32_t i = from; i != to; i += inc) {
            if (fread(scln, 1, scln_sz, file) < scln_sz) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                fclose(file);
                free(dt);
                free(scln);
                return -1;
            }
            for (int32_t j = 0; j < w; ++j) {
                uint16_t pix16 = le16toh(((uint16_t *)scln)[j]);
                for (size_t k = 0; k < channels; ++k) {
                    size_t dt_i = channels*w*i + channels*j + k;
                    uint32_t comp = (pix16 & mask_rgba[k]) >> lshf_rgba[k];
                    uint32_t scale = 1 << diff_rgba[k];
                    dt[dt_i] = comp * scale + comp % scale;
                }
            }
        }
    } break;

    case 24: {
        if (compr != YF_BMP_COMPR_RGB) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            fclose(file);
            free(dt);
            return -1;
        }
        size_t padding = w % 4;
        size_t scln_sz = 3 * w + padding;
        if ((scln = malloc(scln_sz)) == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            fclose(file);
            free(dt);
            return -1;
        }
        size_t k[3] = {2, 1, 0};
        if (le32toh(0xff) != 0xff) {
            k[0] = 0;
            k[2] = 2;
        }
        for (int32_t i = from; i != to; i += inc) {
            if (fread(scln, 1, scln_sz, file) < scln_sz) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                fclose(file);
                free(dt);
                free(scln);
                return -1;
            }
            for (int32_t j = 0; j < w; ++j) {
                size_t dt_i = channels*w*i + channels*j;
                dt[dt_i++] = scln[3*j+k[0]];
                dt[dt_i++] = scln[3*j+k[1]];
                dt[dt_i++] = scln[3*j+k[2]];
            }
        }
    } break;

    case 32: {
        if (compr == YF_BMP_COMPR_RGB) {
            mask_rgba[0] = 0x00ff0000;
            mask_rgba[1] = 0x0000ff00;
            mask_rgba[2] = 0x000000ff;
        }
        for (size_t i = 0; i < channels; ++i)
            YF_SETLSHF(lshf_rgba[i], mask_rgba[i], bpp);
        /* no padding needed */
        size_t scln_sz = w << 2;
        if ((scln = malloc(scln_sz)) == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            fclose(file);
            free(dt);
            return -1;
        }
        for (int32_t i = from; i != to; i += inc) {
            if (fread(scln, 1, scln_sz, file) < scln_sz) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                fclose(file);
                free(dt);
                free(scln);
                return -1;
            }
            for (int32_t j = 0; j < w; ++j) {
                uint32_t pix32 = le32toh(((uint32_t *)scln)[j]);
                for (size_t k = 0; k < channels; ++k) {
                    size_t dt_i = channels*w*i + channels*j + k;
                    dt[dt_i] = (pix32 & mask_rgba[k]) >> lshf_rgba[k];
                }
            }
        }
    } break;

    case 1:
    case 2:
    case 4:
    case 64:
        yf_seterr(YF_ERR_UNSUP, __func__);
        fclose(file);
        free(dt);
        return -1;

    default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        fclose(file);
        free(dt);
        return -1;
    }

    fclose(file);
    free(scln);

    data->data = dt;
    data->pixfmt = channels == 4 ? YF_PIXFMT_RGBA8UNORM : YF_PIXFMT_RGB8UNORM;
    data->dim.width = w;
    data->dim.height = h < 0 ? -h : h;

    return 0;
}
