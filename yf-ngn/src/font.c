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
#include <yf/com/yf-hashset.h>
#include <yf/com/yf-error.h>

#include "font.h"
#include "data-sfnt.h"
#include "texture.h"

struct YF_font_o {
  YF_fontdt data;
  YF_hashset glyphs;
  uint16_t pt;
  uint16_t dpi;
};

/* Type defining key/value for the glyph hashset. */
typedef struct {
  uint16_t key;
  YF_glyph val;
} T_kv_glyph;

/* Glyph hashset functions. */
static size_t hash_glyph(const void *x);
static int cmp_glyph(const void *a, const void *b);
static int deinit_glyph(void *val, void *arg);

YF_font yf_font_init(int filetype, const char *pathname) {
  YF_fontdt data = {0};

  switch (filetype) {
  case YF_FILETYPE_INTERNAL:
    /* TODO */
    assert(0);
    return NULL;
  case YF_FILETYPE_TTF:
    if (yf_loadsfnt(pathname, &data) != 0)
      return NULL;
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

    yf_hashset_each(font->glyphs, deinit_glyph, NULL);
    yf_hashset_deinit(font->glyphs);

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

  font->glyphs = yf_hashset_init(hash_glyph, cmp_glyph);
  if (font->glyphs == NULL) {
    free(font);
    return NULL;
  }
  return font;
}

int yf_font_rasterize(YF_font font, const wchar_t *str, uint16_t pt,
    uint16_t dpi, YF_fontrz *rz)
{
  assert(font != NULL);
  assert(str != NULL && wcslen(str) != 0);
  assert(pt != 0 && dpi != 0);
  assert(rz != NULL);

  /* do not discard glyphs if using the same scale */
  if (pt != font->pt || dpi != font->dpi) {
    yf_hashset_each(font->glyphs, deinit_glyph, NULL);
    yf_hashset_clear(font->glyphs);
    font->pt = pt;
    font->dpi = dpi;
  }

  /* XXX: Caller must ensure 'str' is not empty. */
  const size_t len = wcslen(str);
  struct { uint16_t code; YF_off2 off; } chrs[len];
  size_t chr_i = 0;
  YF_off2 off = {0};
  YF_dim2 dim = {0};
  T_kv_glyph key = {0};
  int16_t y_min, y_max;
  font->data.metrics(font->data.font, pt, dpi, NULL, &y_min, NULL, &y_max);

  for (size_t i = 0; i < len; ++i) {
    /* TODO: Other special characters. */
    switch (str[i]) {
    case '\n':
      dim.height += y_max-y_min;
      off.x = 0;
      off.y = dim.height;
      continue;
    default:
      break;
    }

    key.key = str[i];
    T_kv_glyph *glyph = yf_hashset_search(font->glyphs, &key);

    if (glyph == NULL) {
      glyph = malloc(sizeof *glyph);
      if (glyph == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
      }
      glyph->key = str[i];
      if (yf_hashset_insert(font->glyphs, glyph) != 0)
        return -1;
      /* TODO: Consider ignoring failure here. */
      if (font->data.glyph(font->data.font, str[i], pt, dpi, &glyph->val) != 0)
        return -1;
    }

    chrs[chr_i].code = str[i];
    chrs[chr_i].off = off;
    ++chr_i;

    /* XXX: Should use other metrics to compute spacing. */
    const int16_t extent = YF_MAX(glyph->val.adv_wdt - glyph->val.lsb,
        glyph->val.width);
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
    dim.height += y_max-y_min;
  else
    off.y -= y_max-y_min;

  /* TODO: Use shared textures instead. */
  if (rz->tex != NULL) {
    yf_texture_deinit(rz->tex);
    rz->tex = NULL;
  }
  rz->off = (YF_off2){0};
  rz->dim = dim;

  YF_texdt data;
  data.dim = dim;
  switch (((T_kv_glyph *)yf_hashset_next(font->glyphs, NULL))->val.bpp) {
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
    return -1;
  }
  rz->tex = yf_texture_initdt(&data);
  free(data.data);
  if (rz->tex == NULL)
    return -1;

  /* TODO: Consider copying glyphs to 'data' buffer instead. */
  const YF_off2 bias = {rz->off.x, rz->off.y + off.y};
  for (size_t i = 0; i < chr_i; ++i) {
    key.key = chrs[i].code;
    T_kv_glyph *glyph = yf_hashset_search(font->glyphs, &key);

    if (glyph->val.bitmap.u8 == NULL)
      continue;

    off.x = bias.x + chrs[i].off.x;
    off.y = bias.y - chrs[i].off.y + (glyph->val.base_h - y_min);
    dim = (YF_dim2){glyph->val.width, glyph->val.height};

    if (yf_texture_setdata(rz->tex, off, dim, glyph->val.bitmap.u8) != 0)
      return -1;
  }
  return 0;
}

void yf_font_yieldrz(YF_font font, YF_fontrz *rz) {
  assert(font != NULL);
  assert(rz != NULL);

  /* TODO: Shared textures. */
  yf_texture_deinit(rz->tex);
  memset(rz, 0, sizeof *rz);
}

static size_t hash_glyph(const void *x) {
  return ((T_kv_glyph *)x)->key ^ 21221;
}

static int cmp_glyph(const void *a, const void *b) {
  return ((T_kv_glyph *)a)->key - ((T_kv_glyph *)b)->key;
}

static int deinit_glyph(void *val, YF_UNUSED void *arg) {
  free(((T_kv_glyph *)val)->val.bitmap.u8);
  free(val);
  return 0;
}
