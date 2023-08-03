/*
 * YF
 * test-font.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "test.h"
#include "font.h"

#define YF_WDTMAX 1000
#define YF_HGTMAX 1000

struct font { uint16_t bpp; };

static int create_glyph(void *font, wchar_t code, uint16_t pt, uint16_t dpi,
                        yf_glyph_t *glyph)
{
    assert(font != NULL);

    struct font *fnt = font;
    float scale = (float)(pt*dpi) / (1000.0f*72.0f);
    if (scale > 1.0f)
        scale = 1.0f;

    glyph->width = scale * YF_WDTMAX;
    glyph->height = scale * YF_HGTMAX;
    glyph->bpp = fnt->bpp;
    glyph->base_h = -(glyph->width >> 1);
    glyph->adv_wdt = glyph->width;
    glyph->lsb = 0;

    if (fnt->bpp == 16) {
        size_t sz = (glyph->width * glyph->height) << 1;
        if ((glyph->bm16 = malloc(sz)) == NULL)
            return -1;
        memset(glyph->bm16, 255, sz);
    } else if (fnt->bpp == 8) {
        size_t sz = glyph->width * glyph->height;
        if ((glyph->bm8 = malloc(sz)) == NULL)
            return -1;
        memset(glyph->bm8, 255, sz);
    } else {
        assert(0);
    }

    printf("\n glyph (code=%lc, pt=%hu, dpi=%hu):\n"
           "  width: %hu\n"
           "  height: %hu\n"
           "  glyph->bpp: %hu\n"
           "  base_h: %hd\n"
           "  adv_wdt: %hu\n"
           "  lsb: %hd\n\n",
           code, pt, dpi, glyph->width, glyph->height, glyph->bpp,
           glyph->base_h, glyph->adv_wdt, glyph->lsb);

    return 0;
}

static void get_metrics(void *font, uint16_t pt, uint16_t dpi,
                        int16_t *x_min, int16_t *y_min,
                        int16_t *x_max, int16_t *y_max)
{
    assert(font != NULL);

    float scale = (float)(pt*dpi) / (1000.0f*72.0f);
    if (scale > 1.0f)
        scale = 1.0f;

    printf("\n metrics (pt=%hu, dpi=%hu):\n", pt, dpi);

    if (x_min != NULL) {
        *x_min = -roundf((YF_WDTMAX>>1) * scale);
        printf("  min x: %hd\n", *x_min);
    } else {
        puts("  (no min x)");
    }

    if (y_min != NULL) {
        *y_min = -roundf((YF_HGTMAX>>1) * scale);
        printf("  min y: %hd\n", *y_min);
    } else {
        puts("  (no min y)");
    }

    if (x_max != NULL) {
        *x_max = roundf((YF_WDTMAX>>1) * scale);
        printf("  max x: %hd\n", *x_max);
    } else {
        puts("  (no max x)");
    }

    if (y_max != NULL) {
        *y_max = roundf((YF_HGTMAX>>1) * scale);
        printf("  max y: %hd\n", *y_max);
    } else {
        puts("  (no max y)");
    }

    puts("");
}

static void deinit_font(void *font)
{
    assert(font != NULL);
    puts("\n deinit (font imp.)\n");
}

/* Tests font. */
int yf_test_font(void)
{
    yf_fontdt_t data = {
        .font = &(struct font){8},
        .glyph = create_glyph,
        .metrics = get_metrics,
        .deinit = deinit_font
    };

    YF_TEST_PRINT("init", "&data", "font");
    yf_font_t *font = yf_font_init(&data);
    if (font == NULL)
        return -1;

    yf_fontrz_t rz = {0};

    YF_TEST_PRINT("rasterize", "font, L\"xy12\", 13, 72, &rz", "");
    if (yf_font_rasterize(font, L"xy12", 13, 72, &rz) != 0)
        return -1;

    YF_TEST_PRINT("yieldrz", "font, &rz", "");
    yf_font_yieldrz(font, &rz);

    YF_TEST_PRINT("rasterize", "font, L\"XY345\", 16, 72, &rz", "");
    if (yf_font_rasterize(font, L"XY345", 16, 72, &rz) != 0)
        return -1;

    ((struct font *)data.font)->bpp = 16;

    YF_TEST_PRINT("rasterize", "font, L\"cD6\", 12, 72, &rz", "");
    if (yf_font_rasterize(font, L"cD6", 12, 72, &rz) != 0)
        return -1;

    YF_TEST_PRINT("yieldrz", "font, &rz", "");
    yf_font_yieldrz(font, &rz);

    YF_TEST_PRINT("deinit", "font", "");
    yf_font_deinit(font);

    return 0;
}
