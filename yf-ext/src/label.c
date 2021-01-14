/*
 * YF
 * label.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>

#include <yf/com/yf-util.h>
#include <yf/com/yf-error.h>

#include "label.h"
#include "node.h"
#include "mesh.h"
#include "texture.h"
#include "font.h"
#include "vertex.h"

/* TODO: Consider taking this values from the font instead. */
#define YF_FONTSZ_MIN 9
#define YF_FONTSZ_MAX 144
#ifndef YF_DPI
# define YF_DPI 72
#endif

struct YF_label_o {
  YF_node node;
  YF_mat4 xform;
  YF_mesh mesh;
  YF_texture tex;
  YF_font font;
  wchar_t *str;
  unsigned short pt;
  /* TODO: Other label properties. */
  YF_mat4 mvp;
};

/* Initializes a label's mesh rectangle. */
static int init_rect(YF_label labl);

/* Copies label glyphs to texture. */
static int copy_glyphs(YF_label labl);

YF_label yf_label_init(void) {
  YF_label labl = calloc(1, sizeof(struct YF_label_o));
  if (labl == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  if ((labl->node = yf_node_init()) == NULL) {
    free(labl);
    return NULL;
  }
  yf_node_setobj(labl->node, YF_NODEOBJ_LABEL, labl);
  yf_mat4_iden(labl->xform);
  yf_mat4_iden(labl->mvp);
  labl->pt = 16;

  if (init_rect(labl) != 0) {
    yf_label_deinit(labl);
    return NULL;
  }
  return labl;
}

YF_node yf_label_getnode(YF_label labl) {
  assert(labl != NULL);
  return labl->node;
}

YF_mat4 *yf_label_getxform(YF_label labl) {
  assert(labl != NULL);
  return &labl->xform;
}

YF_mesh yf_label_getmesh(YF_label labl) {
  assert(labl != NULL);
  return labl->mesh;
}

YF_texture yf_label_gettex(YF_label labl) {
  assert(labl != NULL);
  return labl->tex;
}

YF_font yf_label_getfont(YF_label labl) {
  assert(labl != NULL);
  return labl->font;
}

void yf_label_setfont(YF_label labl, YF_font font) {
  assert(labl != NULL);
  labl->font = font;
}

wchar_t *yf_label_getstr(YF_label labl, wchar_t *dst, size_t n) {
  assert(labl != NULL);
  assert(dst != NULL && n > 0);

  if (labl->str == NULL) {
    dst[0] = L'\0';
    return dst;
  }
  if (wcslen(labl->str) < n)
    return wcscpy(dst, labl->str);
  return NULL;
}

int yf_label_setstr(YF_label labl, wchar_t *str) {
  assert(labl != NULL);

  if (str == NULL) {
    free(labl->str);
    labl->str = NULL;
    return 0;
  }

  const size_t n = labl->str == NULL ? 0 : wcslen(labl->str);
  const size_t new_n = wcslen(str);

  if (n != new_n) {
    void *tmp = realloc(labl->str, sizeof(wchar_t) * (new_n + 1));
    if (tmp != NULL) {
      labl->str = tmp;
    } else if (n < new_n) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
  }
  wcscpy(labl->str, str);
  return 0;
}

unsigned short yf_label_getpt(YF_label labl) {
  assert(labl != NULL);
  return labl->pt;
}

int yf_label_setpt(YF_label labl, unsigned short pt) {
  assert(labl != NULL);

  if (pt < YF_FONTSZ_MIN || pt > YF_FONTSZ_MAX) {
    yf_seterr(YF_ERR_LIMIT, __func__);
    return -1;
  }
  labl->pt = pt;
  return 0;
}

void yf_label_deinit(YF_label labl) {
  if (labl != NULL) {
    yf_node_deinit(labl->node);
    yf_mesh_deinit(labl->mesh);
    free(labl->str);
    free(labl);
  }
}

YF_mat4 *yf_label_getmvp(YF_label labl) {
  assert(labl != NULL);
  return &labl->mvp;
}

static int init_rect(YF_label labl) {
  assert(labl != NULL);

  YF_meshdt data = {
    .v = {YF_VTYPE_LABL, NULL, 4},
    .i = {NULL, sizeof(unsigned short), 6}
  };
  data.v.data = malloc(sizeof(YF_vlabl) * data.v.n);
  data.i.data = malloc(data.i.stride * data.i.n);
  if (data.v.data == NULL || data.i.data == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(data.v.data);
    free(data.i.data);
    return -1;
  }

  static const YF_vlabl verts[4] = {
    {
      .pos = {-1.0, -1.0, 0.5},
      .tc = {0.0, 1.0,},
      .clr = {1.0, 1.0, 1.0, 1.0}
    },
    {
      .pos = {-1.0, 1.0, 0.5},
      .tc = {0.0, 0.0},
      .clr = {1.0, 1.0, 1.0, 1.0}
    },
    {
      .pos = {1.0, 1.0, 0.5},
      .tc = {1.0, 0.0},
      .clr = {1.0, 1.0, 1.0, 1.0}
    },
    {
      .pos = {1.0, -1.0, 0.5},
      .tc = {1.0, 1.0},
      .clr = {1.0, 1.0, 1.0, 1.0}
    }
  };
  static const unsigned short inds[6] = {0, 1, 2, 0, 2, 3};

  memcpy(data.v.data, verts, sizeof verts);
  memcpy(data.i.data, inds, sizeof inds);
  labl->mesh = yf_mesh_initdt(&data);
  free(data.v.data);
  free(data.i.data);
  return labl->mesh == NULL ? -1 : 0;
}

static int copy_glyphs(YF_label labl) {
  assert(labl != NULL);
  assert(labl->font != NULL);
  assert(labl->str != NULL);

  const size_t len = wcslen(labl->str);
  if (len == 0)
    return 0;

  YF_glyph glyphs[len];
  YF_dim2 dim = {0};

  /* TODO: Filter glyphs used more than once. */
  for (size_t i = 0; i < len; ++i) {
    if (yf_font_getglyph(labl->font, labl->str[i], labl->pt, YF_DPI, glyphs+i)
        != 0)
    {
      for (size_t j = 0; j < i; ++j)
        free(glyphs[j].bitmap.u8);
      return -1;
    }
    /* TODO: Handle whitespace/newline/etc. */
    dim.width += glyphs[i].width;
    dim.height = YF_MAX(dim.height, glyphs[i].height);
  }

  /* TODO: Use shared textures instead. */
  if (labl->tex != NULL) {
    yf_texture_deinit(labl->tex);
    labl->tex = NULL;
  }

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

  unsigned char *b = data.data;
  for (size_t i = 0; i < len; ++i) {
    size_t sz = (glyphs[i].width * glyphs[i].height) << (glyphs[i].bpp == 16);
    memcpy(b, glyphs[i].bitmap.u8, sz);
    free(glyphs[i].bitmap.u8);
    b += sz;
  }

  labl->tex = yf_texture_initdt(&data);
  free(data.data);
  return labl->tex == NULL ? -1 : 0;
}
