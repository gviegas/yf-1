/*
 * YF
 * font.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "font.h"
#include "data-sfnt.h"

struct YF_font_o {
  YF_fontdt data;
  /* TODO... */
};

YF_font yf_font_init(int filetype, const char *pathname) {
  YF_font font = calloc(1, sizeof(struct YF_font_o));
  if (font == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  switch (filetype) {
    case YF_FILETYPE_INTERNAL:
      /* TODO */
      assert(0);
    case YF_FILETYPE_TTF:
      if (yf_loadsfnt(pathname, &font->data) != 0) {
        yf_font_deinit(font);
        return NULL;
      }
      break;
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      yf_font_deinit(font);
      return NULL;
  }
  return font;
}

void yf_font_deinit(YF_font font) {
  if (font != NULL) {
    if (font->data.deinit != NULL)
      font->data.deinit(font->data.font);
    free(font);
  }
}

int yf_font_getglyph(YF_font font, wchar_t code, uint16_t pts, uint16_t dpi,
    YF_glyph *glyph)
{
  assert(font != NULL);
  assert(pts != 0 && dpi != 0);
  assert(glyph != NULL);

  return font->data.glyph(font->data.font, code, pts, dpi, glyph);
}
