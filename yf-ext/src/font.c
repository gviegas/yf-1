/*
 * YF
 * font.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>

#include <yf/com/yf-util.h>
#include <yf/com/yf-error.h>

#include "font.h"
#include "data-sfnt.h"
#include "texture.h"

struct YF_font_o {
  YF_fontdt data;
  /* TODO... */
};

YF_font yf_font_init(int filetype, const char *pathname) {
  YF_fontdt data = {0};
  switch (filetype) {
    case YF_FILETYPE_INTERNAL:
      /* TODO */
      assert(0);
    case YF_FILETYPE_TTF:
      if (yf_loadsfnt(pathname, &data) != 0) {
        return NULL;
      }
      break;
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      return NULL;
  }
  YF_font font = yf_font_initdt(&data);
  if (font == NULL && data.deinit != NULL)
    data.deinit(data.font);
  return font;
}

void yf_font_deinit(YF_font font) {
  if (font != NULL) {
    if (font->data.deinit != NULL)
      font->data.deinit(font->data.font);
    free(font);
  }
}

YF_font yf_font_initdt(const YF_fontdt *data) {
  assert(data != NULL);

  YF_font font = calloc(1, sizeof(struct YF_font_o));
  if (font == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  memcpy(&font->data, data, sizeof *data);
  return font;
}

int yf_font_rasterize(YF_font font, wchar_t *str, uint16_t pt, uint16_t dpi,
    YF_fontrz *rz)
{
  assert(font != NULL);
  assert(str != NULL && wcslen(str) != 0);
  assert(pt != 0 && dpi != 0);
  assert(rz != NULL);

  const size_t len = wcslen(str);
/*
  if (len == 0)
    return 0;
*/

  YF_glyph glyphs[len];
  YF_dim2 dim = {0};

  /* TODO: Filter glyphs used more than once. */
  for (size_t i = 0; i < len; ++i) {
    if (font->data.glyph(font->data.font, str[i], pt, dpi, glyphs+i) != 0) {
      for (size_t j = 0; j < i; ++j)
        free(glyphs[j].bitmap.u8);
      return -1;
    }
    /* TODO: Handle whitespace/newline/etc. */
    dim.width += glyphs[i].width;
    dim.height = YF_MAX(dim.height, glyphs[i].height);
  }

  /* TODO: Use shared textures instead. */
  if (rz->tex != NULL) {
    yf_texture_deinit(rz->tex);
    rz->tex = NULL;
  }
  rz->off = (YF_off2){0};
  rz->dim = (YF_dim2){0};

  YF_texdt data;
  data.dim = dim;

  switch (glyphs[0].bpp) {
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
  }

  if (data.data == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    for (size_t i = 0; i < len; ++i)
      free(glyphs[i].bitmap.u8);
    return -1;
  }

  /* XXX: Cannot copy string of glyphs to linear buffer directly. */
/*
  unsigned char *b = data.data;
  for (size_t i = 0; i < len; ++i) {
    size_t sz = (glyphs[i].width * glyphs[i].height) << (glyphs[i].bpp == 16);
    memcpy(b, glyphs[i].bitmap.u8, sz);
    free(glyphs[i].bitmap.u8);
    b += sz;
  }
*/

  rz->tex = yf_texture_initdt(&data);
  free(data.data);

  ////////////////////
  // XXX
  YF_off2 off = {0};
  for (size_t i = 0; i < len; ++i) {
    dim.width = glyphs[i].width;
    dim.height = glyphs[i].height;
    if (yf_texture_setdata(rz->tex, off, dim, glyphs[i].bitmap.u8) != 0)
      assert(0);
    off.x += dim.width;
    rz->dim.width += dim.width;
    rz->dim.height = YF_MAX(rz->dim.height, dim.height);
    free(glyphs[i].bitmap.u8);
  }
  int16_t x[2], y[2];
  font->data.metrics(font->data.font, pt, dpi, x, y, x+1, y+1);
  printf("font metrics: x=[%hd, %hd], y=[%hd, %hd]\n", x[0], x[1], y[0], y[1]);
  ////////////////////

  return rz->tex == NULL ? -1 : 0;
}
