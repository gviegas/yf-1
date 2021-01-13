/*
 * YF
 * label.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <wchar.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "label.h"
#include "node.h"
#include "mesh.h"
#include "vertex.h"

struct YF_label_o {
  YF_node node;
  YF_mat4 xform;
  YF_mesh mesh;
  wchar_t *str;
  /* TODO: Other label properties. */
  YF_mat4 mvp;
};

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

  const size_t n = wcslen(labl->str);
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

void yf_label_deinit(YF_label labl) {
  if (labl != NULL) {
    yf_node_deinit(labl->node);
    free(labl->str);
    free(labl);
  }
}

YF_mat4 *yf_label_getmvp(YF_label labl) {
  assert(labl != NULL);
  return &labl->mvp;
}
