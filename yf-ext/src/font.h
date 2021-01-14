/*
 * YF
 * font.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_FONT_H
#define YF_FONT_H

#include <stdint.h>

#include "yf-font.h"

/* Type defining a font glyph. */
typedef struct {
  uint16_t width;
  uint16_t height;
  /* Greyscale format, either 8 or 16 bits per pixel. */
  uint16_t bpp;
  union {
    uint8_t *u8;
    uint16_t *u16;
  } bitmap;
  /* horizontal metrics */
  uint16_t adv_wdt;
  uint16_t lsb;
} YF_glyph;

/* Type defining the font data. */
typedef struct {
  /* Font implementation. */
  void *font;
  /* Glyph generation. */
  int (*glyph)(void *font, wchar_t code, uint16_t pt, uint16_t dpi,
      YF_glyph *glyph);
  /* Deinitialization. */
  void (*deinit)(void *font);
} YF_fontdt;

/* Gets a font glyph. */
int yf_font_getglyph(YF_font font, wchar_t code, uint16_t pt, uint16_t dpi,
    YF_glyph *glyph);

#endif /* YF_FONT_H */
