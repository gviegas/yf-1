/*
 * YF
 * quad.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "quad.h"
#include "node.h"
#include "mesh.h"
#include "vertex.h"

struct YF_quad_o {
  YF_node node;
  YF_mat4 xform;
  YF_mesh mesh;
  YF_texture tex;
  /* TODO: Other quad properties. */
};

/* Initializes a quad's mesh rectangle. */
static int init_rect(YF_quad quad);

YF_quad yf_quad_init(void) {
  YF_quad quad = calloc(1, sizeof(struct YF_quad_o));
  if (quad == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  if ((quad->node = yf_node_init()) == NULL) {
    free(quad);
    return NULL;
  }
  yf_node_setobj(quad->node, YF_NODEOBJ_QUAD, quad);
  yf_mat4_iden(quad->xform);

  if (init_rect(quad) != 0) {
    yf_quad_deinit(quad);
    return NULL;
  }
  return quad;
}

YF_node yf_quad_getnode(YF_quad quad) {
  assert(quad != NULL);
  return quad->node;
}

YF_mat4 *yf_quad_getxform(YF_quad quad) {
  assert(quad != NULL);
  return &quad->xform;
}

YF_mesh yf_quad_getmesh(YF_quad quad) {
  assert(quad != NULL);
  return quad->mesh;
}

YF_texture yf_quad_gettex(YF_quad quad) {
  assert(quad != NULL);
  return quad->tex;
}

void yf_quad_settex(YF_quad quad, YF_texture tex) {
  assert(quad != NULL);
  quad->tex = tex;
}

void yf_quad_deinit(YF_quad quad) {
  if (quad != NULL) {
    yf_node_deinit(quad->node);
    yf_mesh_deinit(quad->mesh);
    free(quad);
  }
}

static int init_rect(YF_quad quad) {
  assert(quad != NULL);

  YF_meshdt data = {
    .v = {YF_VTYPE_QUAD, NULL, 4},
    .i = {NULL, sizeof(unsigned short), 6}
  };
  data.v.data = malloc(sizeof(YF_vquad) * data.v.n);
  data.i.data = malloc(data.i.stride * data.i.n);
  if (data.v.data == NULL || data.i.data == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(data.v.data);
    free(data.i.data);
    return -1;
  }

  static const YF_vquad verts[4] = {
    {
      .pos = {-1.0, -1.0, 0.5},
      .tc = {0.0, 1.0},
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
  quad->mesh = yf_mesh_initdt(&data);
  free(data.v.data);
  free(data.i.data);
  return quad->mesh == NULL ? -1 : 0;
}
