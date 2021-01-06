/*
 * YF
 * font.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
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
} YF_glyph;

/* Type defining the font data. */
typedef struct {
  /* Font implementation. */
  void *font;
  /* Glyph generation. */
  YF_glyph *(*glyph_fn)(void *font, wchar_t code, uint16_t pts, uint16_t dpi);
  /* Deinitialization. */
  int (*deinit_fn)(void *font);
} YF_fontdt;

#endif /* YF_FONT_H */
