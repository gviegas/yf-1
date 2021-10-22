/*
 * YF
 * font.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_FONT_H
#define YF_FONT_H

#include "yf/com/yf-types.h"

#include "yf-font.h"
#include "yf-texture.h"

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
