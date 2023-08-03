/*
 * YF
 * font.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_FONT_H
#define YF_FONT_H

#include "yf/com/yf-types.h"

#include "yf-font.h"
#include "yf-texture.h"

/* Font rasterization output. */
typedef struct yf_fontrz {
    yf_texture_t *tex;
    yf_off2_t off;
    yf_dim2_t dim;
} yf_fontrz_t;

/* Rasterizes font glyphs.
   The 'rz' structure will contain the texture range for a 'str' bitmap.
   When the 'tex' member of 'rz' refers to a valid texture, this function
   assumes that the current range is no longer needed. */
int yf_font_rasterize(yf_font_t *font, const wchar_t *str, uint16_t pt,
                      uint16_t dpi, yf_fontrz_t *rz);

/* Yields a texture range obtained from a call to 'font_rasterize()'.
   This function must be called when a given texture range is no longer needed.
   When replacing the contents of a previous rasterization, one should instead
   pass the 'rz' structure unmodified to 'font_rasterize()'. */
void yf_font_yieldrz(yf_font_t *font, yf_fontrz_t *rz);

#endif /* YF_FONT_H */
