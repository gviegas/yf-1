/*
 * YF
 * yf-font.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_FONT_H
#define YF_YF_FONT_H

#include <stddef.h>
#include <stdint.h>

#include "yf/com/yf-defs.h"

#include "yf-collection.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a font resource.
 */
typedef struct YF_font_o *YF_font;

/**
 * Loads a new font from file.
 *
 * @param pathname: The pathname of the font file.
 * @param index: The index of the font to load.
 * @param coll: The collection for the font.
 * @return: On success, returns a new font. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_font yf_font_load(const char *pathname, size_t index, YF_collection coll);

/**
 * Type defining a font glyph.
 */
typedef struct {
    uint16_t width;
    uint16_t height;

    /* Greyscale format, either 8 or 16 bits per pixel. */
    uint16_t bpp;
    union {
        uint8_t *u8;
        uint16_t *u16;
    } bitmap;

    /* Horizontal metrics. */
    int16_t base_h;
    uint16_t adv_wdt;
    int16_t lsb;
} YF_glyph;

/**
 * Type defining font data.
 */
typedef struct {
    /* Font implementation. */
    void *font;

    /* Glyph generation. */
    int (*glyph)(void *font, wchar_t code, uint16_t pt, uint16_t dpi,
                 YF_glyph *glyph);

    /* General metrics, scaled to given 'pt' and 'dpi' values. */
    /* TODO: Other metrics besides bbox. */
    void (*metrics)(void *font, uint16_t pt, uint16_t dpi,
                    int16_t *x_min, int16_t *y_min,
                    int16_t *x_max, int16_t *y_max);

    /* Deinitialization. */
    void (*deinit)(void *font);
} YF_fontdt;

/**
 * Initializes a new font.
 *
 * @param data: The data from which to initialize the font.
 * @return: On success, returns a new font. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_font yf_font_init(const YF_fontdt *data);

/**
 * Deinitializes a font.
 *
 * @param font: The font to deinitialize. Can be 'NULL'.
 */
void yf_font_deinit(YF_font font);

YF_DECLS_END

#endif /* YF_YF_FONT_H */
