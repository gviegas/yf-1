/*
 * YF
 * font.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>

#include "yf/com/yf-util.h"
#include "yf/com/yf-dict.h"
#include "yf/com/yf-error.h"

#include "font.h"
#include "data-sfnt.h"
#include "texture.h"

struct yf_font {
    yf_fontdt_t data;
    yf_dict_t *glyphs;
    uint16_t pt;
    uint16_t dpi;
};

/* Deinitializes a glyph. */
static int deinit_glyph(YF_UNUSED void *key, void *val, YF_UNUSED void *arg)
{
    free(((yf_glyph_t *)val)->bm8);
    free(val);
    return 0;
}

yf_font_t *yf_font_load(const char *pathname, size_t index, yf_collec_t *coll)
{
    /* TODO: Consider checking the type of the file. */
    if (coll == NULL)
        coll = yf_collec_get();
    return yf_collec_loaditem(coll, YF_CITEM_FONT, pathname, index);
}

yf_font_t *yf_font_init(const yf_fontdt_t *data)
{
    assert(data != NULL);

    yf_font_t *font = calloc(1, sizeof(yf_font_t));
    if (font == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    memcpy(&font->data, data, sizeof *data);
    font->glyphs = yf_dict_init(NULL, NULL);

    if (font->glyphs == NULL) {
        free(font);
        return NULL;
    }

    return font;
}

void yf_font_deinit(yf_font_t *font)
{
    if (font == NULL)
        return;

    if (font->data.deinit != NULL)
        font->data.deinit(font->data.font);

    yf_dict_each(font->glyphs, deinit_glyph, NULL);
    yf_dict_deinit(font->glyphs);

    free(font);
}

int yf_font_rasterize(yf_font_t *font, const wchar_t *str, uint16_t pt,
                      uint16_t dpi, yf_fontrz_t *rz)
{
    assert(font != NULL);
    assert(str != NULL && wcslen(str) != 0);
    assert(pt != 0 && dpi != 0);
    assert(rz != NULL);

    /* do not discard glyphs if using the same scale */
    if (pt != font->pt || dpi != font->dpi) {
        yf_dict_each(font->glyphs, deinit_glyph, NULL);
        yf_dict_clear(font->glyphs);
        font->pt = pt;
        font->dpi = dpi;
    }

    /* XXX: Caller must ensure 'str' is not empty. */
    const size_t len = wcslen(str);
    struct { uint16_t code; yf_off2_t off; } chrs[len];
    size_t chr_i = 0;
    yf_off2_t off = {0};
    yf_dim2_t dim = {0};
    int16_t y_min, y_max;
    font->data.metrics(font->data.font, pt, dpi, NULL, &y_min, NULL, &y_max);

    for (size_t i = 0; i < len; i++) {
        /* TODO: Other special characters. */
        switch (str[i]) {
        case '\n':
            dim.height += y_max - y_min;
            off.x = 0;
            off.y = dim.height;
            continue;
        default:
            break;
        }

        const void *key = (void *)(uintptr_t)str[i];
        yf_glyph_t *glyph = yf_dict_search(font->glyphs, key);

        if (glyph == NULL) {
            glyph = malloc(sizeof *glyph);

            if (glyph == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                return -1;
            }

            if (yf_dict_insert(font->glyphs, key, glyph) != 0) {
                free(glyph);
                return -1;
            }

            /* TODO: Consider ignoring failure here. */
            if (font->data.glyph(font->data.font, str[i], pt, dpi, glyph) != 0)
                return -1;
        }

        chrs[chr_i].code = str[i];
        chrs[chr_i].off = off;
        chr_i++;

        /* XXX: Should use other metrics to compute spacing. */
        const int16_t extent = YF_MAX(glyph->adv_wdt - glyph->lsb,
                                      glyph->width);
        off.x += extent;
        dim.width = YF_MAX(dim.width, off.x);
    }

    if (chr_i == 0) {
        /* no characters to rasterize */
        yf_seterr(YF_ERR_OTHER, __func__);
        memset(rz, 0, sizeof *rz);
        return -1;
    }

    if (off.x != 0)
        /* last valid character is not eol */
        dim.height += y_max - y_min;
    else
        off.y -= y_max-y_min;

    /* TODO: Use shared textures instead. */
    if (rz->tex != NULL) {
        yf_texture_deinit(rz->tex);
        rz->tex = NULL;
    }

    rz->off = (yf_off2_t){0};
    rz->dim = dim;
    /* XXX: Sampler params. set to zero. */
    yf_texdt_t data = {0};
    data.dim = dim;

    switch (((yf_glyph_t *)yf_dict_next(font->glyphs, NULL, NULL))->bpp) {
    case 8:
        data.pixfmt = YF_PIXFMT_R8UNORM;
        data.data = calloc(1, dim.width * dim.height);
        break;
    case 16:
        data.pixfmt = YF_PIXFMT_R16UNORM;
        data.data = calloc(1, (dim.width * dim.height) << 1);
        break;
    default:
        assert(0);
        yf_seterr(YF_ERR_OTHER, __func__);
        return -1;
    }

    if (data.data == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    rz->tex = yf_texture_init(&data);
    free(data.data);

    if (rz->tex == NULL)
        return -1;

    const yf_off2_t bias = {rz->off.x, rz->off.y + off.y};

    /* TODO: Consider copying glyphs to 'data' buffer instead. */
    for (size_t i = 0; i < chr_i; i++) {
        const uintptr_t code = chrs[i].code;
        yf_glyph_t *glyph = yf_dict_search(font->glyphs, (void *)code);

        if (glyph->bm8 == NULL)
            continue;

        off.x = bias.x + chrs[i].off.x;
        off.y = bias.y - chrs[i].off.y + (glyph->base_h - y_min);
        dim = (yf_dim2_t){glyph->width, glyph->height};

        if (yf_texture_setdata(rz->tex, off, dim, glyph->bm8) != 0)
            return -1;
    }

    return 0;
}

void yf_font_yieldrz(yf_font_t *font, yf_fontrz_t *rz)
{
    assert(font != NULL);
    assert(rz != NULL);

    /* TODO: Shared textures. */
    yf_texture_deinit(rz->tex);
    memset(rz, 0, sizeof *rz);
}
