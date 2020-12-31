/*
 * YF
 * font.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "font.h"

struct YF_font_o {
  /* TODO */
  int _;
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
      /* TODO */
      assert(0);
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      yf_font_deinit(font);
      return NULL;
  }
  /* TODO */
  return font;
}

void yf_font_deinit(YF_font font) {
  if (font != NULL) {
    /* TODO */
    free(font);
  }
}
