/*
 * YF
 * font.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_FONT_H
#define YF_FONT_H

#include <stdint.h>

#include "yf/com/yf-types.h"

#include "yf-font.h"
#include "yf-texture.h"

/* Font glyph. */
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

/* Font data. */
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

/* Initializes a new font object from font data directly. */
YF_font yf_font_initdt(const YF_fontdt *data);

/* Font rasterization output. */
typedef struct {
    YF_texture tex;
    YF_off2 off;
    YF_dim2 dim;
} YF_fontrz;

/* Rasterizes font glyphs.
   The 'rz' structure will contain the texture range for a 'str' bitmap.
   When the 'tex' member of 'rz' refers to a valid texture, this function
   assumes that the current range is no longer needed. */
int yf_font_rasterize(YF_font font, const wchar_t *str, uint16_t pt,
                      uint16_t dpi, YF_fontrz *rz);

/* Yields a texture range obtained from a call to 'font_rasterize()'.
   This function must be called when a given texture range is no longer needed.
   When replacing the contents of a previous rasterization, one should instead
   pass the 'rz' structure unmodified to 'font_rasterize()'. */
void yf_font_yieldrz(YF_font font, YF_fontrz *rz);

#endif /* YF_FONT_H */
