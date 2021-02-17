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
  YF_vlabl verts[4];
  YF_mesh mesh;
  YF_fontrz rz;
  YF_font font;
  wchar_t *str;
  unsigned short pt;
#define YF_PEND_NONE 0
#define YF_PEND_TC   0x01 /* 'rz' changed, 'verts[].tc' not up to date */
#define YF_PEND_CLR  0x02 /* 'verts[].clr' set but 'mesh' not up to date */
#define YF_PEND_RZ   0x04 /* font prop. changed, 'rz' not up to date */
  unsigned pend_mask;
  /* TODO: Other label properties. */
};

/* Initializes a label's mesh rectangle. */
static int init_rect(YF_label labl);

/* Updates a label's mesh rectangle. */
static void update_rect(YF_label labl);

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
  labl->pt = 16;
  labl->pend_mask = YF_PEND_RZ;

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

YF_mesh yf_label_getmesh(YF_label labl) {
  assert(labl != NULL);

  if (labl->pend_mask != YF_PEND_NONE) {
    if (labl->pend_mask & YF_PEND_RZ) {
      copy_glyphs(labl);
      labl->pend_mask &= ~YF_PEND_RZ;
      labl->pend_mask |= YF_PEND_TC;
    }
    update_rect(labl);
    labl->pend_mask = YF_PEND_NONE;
  }
  return labl->mesh;
}

YF_texture yf_label_gettex(YF_label labl) {
  assert(labl != NULL);

  if (labl->pend_mask & YF_PEND_RZ) {
    copy_glyphs(labl);
    labl->pend_mask &= ~YF_PEND_RZ;
    labl->pend_mask |= YF_PEND_TC;
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
  labl->pend_mask |= YF_PEND_RZ;
}

wchar_t *yf_label_getstr(YF_label labl, wchar_t *dst, size_t n) {
  assert(labl != NULL);
  assert(dst != NULL);
  assert(n > 0);

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
    labl->pend_mask |= YF_PEND_RZ;
    return 0;
  }

  const size_t len = wcslen(str);
  if (labl->str == NULL) {
    labl->str = malloc(sizeof(wchar_t) * (1+len));
    if (labl->str == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
  } else {
    const size_t cur_len = wcslen(labl->str);
    if (cur_len != len) {
      void *tmp = realloc(labl->str, sizeof(wchar_t) * (1+len));
      if (tmp != NULL) {
        labl->str = tmp;
      } else if (cur_len < len) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
      }
    }
  }
  wcscpy(labl->str, str);
  labl->pend_mask |= YF_PEND_RZ;
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
  labl->pend_mask |= YF_PEND_RZ;
  return 0;
}

YF_color yf_label_getcolor(YF_label labl, int corner) {
  assert(labl != NULL);

  unsigned i = 0;
  switch (corner) {
    case YF_CORNER_TOPL:
    case YF_CORNER_TOP:
    case YF_CORNER_LEFT:
    case YF_CORNER_ALL:
      i = 0;
      break;
    case YF_CORNER_TOPR:
    case YF_CORNER_RIGHT:
      i = 3;
      break;
    case YF_CORNER_BOTTOML:
    case YF_CORNER_BOTTOM:
      i = 1;
      break;
    case YF_CORNER_BOTTOMR:
      i = 2;
      break;
  }

  const YF_vlabl *v = labl->verts+i;
  return (YF_color){v->clr[0], v->clr[1], v->clr[2], v->clr[3]};
}

void yf_label_setcolor(YF_label labl, unsigned corner_mask, YF_color color) {
  assert(labl != NULL);

  if (corner_mask & YF_CORNER_TOPL) {
    labl->verts[0].clr[0] = color.r;
    labl->verts[0].clr[1] = color.g;
    labl->verts[0].clr[2] = color.b;
    labl->verts[0].clr[3] = color.a;
  }
  if (corner_mask & YF_CORNER_TOPR) {
    labl->verts[3].clr[0] = color.r;
    labl->verts[3].clr[1] = color.g;
    labl->verts[3].clr[2] = color.b;
    labl->verts[3].clr[3] = color.a;
  }
  if (corner_mask & YF_CORNER_BOTTOML) {
    labl->verts[1].clr[0] = color.r;
    labl->verts[1].clr[1] = color.g;
    labl->verts[1].clr[2] = color.b;
    labl->verts[1].clr[3] = color.a;
  }
  if (corner_mask & YF_CORNER_BOTTOMR) {
    labl->verts[2].clr[0] = color.r;
    labl->verts[2].clr[1] = color.g;
    labl->verts[2].clr[2] = color.b;
    labl->verts[2].clr[3] = color.a;
  }

  labl->pend_mask |= YF_PEND_CLR;
}

YF_dim2 yf_label_getdim(YF_label labl) {
  assert(labl != NULL);

  if (labl->pend_mask & YF_PEND_RZ) {
    copy_glyphs(labl);
    labl->pend_mask &= ~YF_PEND_RZ;
    labl->pend_mask |= YF_PEND_TC;
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

static void update_rect(YF_label labl) {
  assert(labl != NULL);
  assert(labl->pend_mask != YF_PEND_NONE);

  if (labl->pend_mask & YF_PEND_TC) {
    const YF_fontrz *rz = &labl->rz;
    YF_float s0, t0, s1, t1;

    if (rz->tex == NULL || rz->dim.width == 0 || rz->dim.height == 0) {
      s0 = t0 = 0.0;
      s1 = t1 = 1.0;
    } else {
      const YF_dim2 dim = yf_texture_getdim(rz->tex);
      const YF_float wdt = dim.width;
      const YF_float hgt = dim.height;
      s0 = rz->off.x / wdt;
      t0 = rz->off.y / hgt;
      s1 = rz->dim.width / wdt + s0;
      t1 = rz->dim.height / hgt + t0;
    }

    labl->verts[0].tc[0] = s0;
    labl->verts[0].tc[1] = t1;

    labl->verts[1].tc[0] = s0;
    labl->verts[1].tc[1] = t0;

    labl->verts[2].tc[0] = s1;
    labl->verts[2].tc[1] = t0;

    labl->verts[3].tc[0] = s1;
    labl->verts[3].tc[1] = t1;
  }

  const YF_slice range = {0, 4};
#ifdef YF_DEVEL
  if (yf_mesh_setvtx(labl->mesh, range, labl->verts) != 0) assert(0);
#else
  yf_mesh_setvtx(labl->mesh, range, labl->verts);
#endif
}

static int copy_glyphs(YF_label labl) {
  assert(labl != NULL);
  assert(labl->font != NULL);
  assert(labl->pend_mask & YF_PEND_RZ);

  const wchar_t *str;
  if (labl->str == NULL)
    str = L"(nil)";
  else if (wcslen(labl->str) == 0)
    str = L"(empty)";
  else
    str = labl->str;

  return yf_font_rasterize(labl->font, str, labl->pt, YF_DPI, &labl->rz);
}
