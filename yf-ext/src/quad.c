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
  YF_vquad verts[4];
  YF_mesh mesh;
  YF_texture tex;
  YF_rect rect;
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
  quad->rect.origin = (YF_off2){0};
  quad->rect.size = tex != NULL ? yf_texture_getdim(tex) : (YF_dim2){0};
}

const YF_rect *yf_quad_getrect(YF_quad quad) {
  assert(quad != NULL);
  return &quad->rect;
}

void yf_quad_setrect(YF_quad quad, const YF_rect *rect) {
  assert(quad != NULL);
  assert(rect != NULL);

  memcpy(&quad->rect, rect, sizeof *rect);
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

  const YF_meshdt data = {
    .v = {YF_VTYPE_QUAD, verts, 4},
    .i = {inds, sizeof(inds[0]), 6}
  };

  quad->mesh = yf_mesh_initdt(&data);
  memcpy(quad->verts, verts, sizeof verts);
  return quad->mesh == NULL ? -1 : 0;
}
