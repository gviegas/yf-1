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
  YF_vlabl verts[4];
  YF_mesh mesh;
  YF_fontrz rz;
  YF_font font;
  wchar_t *str;
  unsigned short pt;
  int changed;
  /* TODO: Other label properties. */
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
  labl->pt = 16;
  labl->changed = 1;

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

  if (labl->changed) {
    copy_glyphs(labl);
    labl->changed = 0;
  }
  return labl->rz.tex;
}

YF_font yf_label_getfont(YF_label labl) {
  assert(labl != NULL);
  return labl->font;
}

void yf_label_setfont(YF_label labl, YF_font font) {
  assert(labl != NULL);

  if (font == labl->font)
    return;
  if (labl->font != NULL)
    yf_font_yieldrz(labl->font, &labl->rz);

  labl->font = font;
  labl->changed = 1;
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

int yf_label_setstr(YF_label labl, const wchar_t *str) {
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
  labl->changed = 1;
  return 0;
}

unsigned short yf_label_getpt(YF_label labl) {
  assert(labl != NULL);
  return labl->pt;
}

int yf_label_setpt(YF_label labl, unsigned short pt) {
  assert(labl != NULL);

  if (pt == labl->pt)
    return 0;

  if (pt < YF_FONTSZ_MIN || pt > YF_FONTSZ_MAX) {
    yf_seterr(YF_ERR_LIMIT, __func__);
    return -1;
  }
  labl->pt = pt;
  labl->changed = 1;
  return 0;
}

YF_color yf_label_getcolor(YF_label labl, int corner) {
  /* TODO */
  assert(0);
}

void yf_label_setcolor(YF_label labl, unsigned corner_mask, YF_color color) {
  /* TODO */
  assert(0);
}

YF_dim2 yf_label_getdim(YF_label labl) {
  assert(labl != NULL);

  if (labl->changed) {
    copy_glyphs(labl);
    labl->changed = 0;
  }
  return labl->rz.dim;
}

void yf_label_deinit(YF_label labl) {
  if (labl != NULL) {
    yf_node_deinit(labl->node);
    yf_mesh_deinit(labl->mesh);

    if (labl->font != NULL)
      yf_font_yieldrz(labl->font, &labl->rz);

    free(labl->str);
    free(labl);
  }
}

static int init_rect(YF_label labl) {
  assert(labl != NULL);

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

  const YF_meshdt data = {
    .v = {YF_VTYPE_LABL, (void *)verts, 4},
    .i = {(void *)inds, sizeof inds[0], 6}
  };

  labl->mesh = yf_mesh_initdt(&data);
  memcpy(labl->verts, verts, sizeof verts);
  return labl->mesh == NULL ? -1 : 0;
}

static int copy_glyphs(YF_label labl) {
  assert(labl != NULL);
  assert(labl->font != NULL);
  assert(labl->str != NULL);

  const size_t len = wcslen(labl->str);
  if (len == 0)
    return 0;

  return yf_font_rasterize(labl->font, labl->str, labl->pt, YF_DPI, &labl->rz);
}
