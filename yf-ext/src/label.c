/*
 * YF
 * label.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "label.h"
#include "node.h"
#include "mesh.h"
#include "vertex.h"
#include "error.h"

struct YF_label_o {
  YF_node node;
  YF_mat4 xform;
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

void yf_label_deinit(YF_label labl) {
  if (labl != NULL) {
    yf_node_deinit(labl->node);
    free(labl);
  }
}

YF_mat4 *yf_label_getmvp(YF_label labl) {
  assert(labl != NULL);
  return &labl->mvp;
}
