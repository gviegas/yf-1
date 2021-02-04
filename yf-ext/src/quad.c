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
  int changed;
  /* TODO: Other quad properties. */
};

/* Initializes a quad's mesh rectangle. */
static int init_rect(YF_quad quad);

/* Updates a quad's mesh rectangle. */
static void update_rect(YF_quad quad);

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

  if (quad->changed) {
    update_rect(quad);
    quad->changed = 0;
  }
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
  quad->changed = 1;
}

const YF_rect *yf_quad_getrect(YF_quad quad) {
  assert(quad != NULL);
  return &quad->rect;
}

void yf_quad_setrect(YF_quad quad, const YF_rect *rect) {
  assert(quad != NULL);
  assert(rect != NULL);

  memcpy(&quad->rect, rect, sizeof *rect);
  quad->changed = 1;
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

static void update_rect(YF_quad quad) {
  assert(quad != NULL);

  YF_float u0, v0, u1, v1;

  if (quad->rect.size.width == 0) {
    u0 = v0 = 0.0;
    u1 = v1 = 1.0;
  } else {
    /* XXX: This assumes that the rect values are valid. */
    assert(quad->tex != NULL);
    const YF_dim2 dim = yf_texture_getdim(quad->tex);
    const YF_float u_max = dim.width;
    const YF_float v_max = dim.height;
    u0 = quad->rect.origin.x / u_max;
    v0 = quad->rect.origin.y / v_max;
    u1 = quad->rect.size.width / u_max + u0;
    v1 = quad->rect.size.height / v_max + v0;
  }

  quad->verts[0].tc[0] = u0;
  quad->verts[0].tc[1] = v1;

  quad->verts[1].tc[0] = u0;
  quad->verts[1].tc[1] = v0;

  quad->verts[2].tc[0] = u1;
  quad->verts[2].tc[1] = v0;

  quad->verts[3].tc[0] = u1;
  quad->verts[3].tc[1] = v1;

  const YF_slice range = {0, 4};
#ifdef YF_DEVEL
  if (yf_mesh_setvtx(quad->mesh, range, quad->verts) != 0) assert(0);
#else
  yf_mesh_setvtx(quad->mesh, range, quad->verts);
#endif
}
