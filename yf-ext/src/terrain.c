/*
 * YF
 * terrain.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "terrain.h"
#include "node.h"
#include "mesh.h"
#include "vertex.h"

struct YF_terrain_o {
  YF_node node;
  unsigned width;
  unsigned depth;
  YF_mat4 xform;
  YF_mesh mesh;
  YF_texture hmap;
  YF_texture tex;
  /* TODO: Other terrain properties. */
  YF_mat4 mvp;
};

/* Initializes grid mesh. */
static int init_grid(YF_terrain terr);

YF_terrain yf_terrain_init(unsigned width, unsigned depth) {
  if (width == 0 || depth == 0) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return NULL;
  }

  YF_terrain terr = calloc(1, sizeof(struct YF_terrain_o));
  if (terr == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  if ((terr->node = yf_node_init()) == NULL) {
    free(terr);
    return NULL;
  }
  yf_node_setobj(terr->node, YF_NODEOBJ_TERRAIN, terr);
  terr->width = width;
  terr->depth = depth;
  yf_mat4_iden(terr->xform);
  yf_mat4_iden(terr->mvp);

  if (init_grid(terr) != 0) {
    yf_terrain_deinit(terr);
    return NULL;
  }
  return terr;
}

YF_node yf_terrain_getnode(YF_terrain terr) {
  assert(terr != NULL);
  return terr->node;
}

YF_mat4 *yf_terrain_getxform(YF_terrain terr) {
  assert(terr != NULL);
  return &terr->xform;
}

YF_mesh yf_terrain_getmesh(YF_terrain terr) {
  assert(terr != NULL);
  return terr->mesh;
}

YF_texture yf_terrain_gethmap(YF_terrain terr) {
  assert(terr != NULL);
  return terr->hmap;
}

void yf_terrain_sethmap(YF_terrain terr, YF_texture hmap) {
  assert(terr != NULL);
  terr->hmap = hmap;
}

YF_texture yf_terrain_gettex(YF_terrain terr) {
  assert(terr != NULL);
  return terr->tex;
}

void yf_terrain_settex(YF_terrain terr, YF_texture tex) {
  assert(terr != NULL);
  terr->tex = tex;
}

void yf_terrain_deinit(YF_terrain terr) {
  if (terr != NULL) {
    yf_node_deinit(terr->node);
    yf_mesh_deinit(terr->mesh);
    free(terr);
  }
}

YF_mat4 *yf_terrain_getmvp(YF_terrain terr) {
  assert(terr != NULL);
  return &terr->mvp;
}

static int init_grid(YF_terrain terr) {
  assert(terr != NULL);

  /* TODO: Support for custom tiling. */

  const unsigned wid = terr->width;
  const unsigned dep = terr->depth;

  YF_meshdt data = {0};
  data.v.vtype = YF_VTYPE_TERR;
  data.v.n = (wid+1) * (dep+1);
  data.i.n = wid * dep * 6;
  if (data.v.n > 65536)
    data.i.stride = sizeof(unsigned);
  else
    data.i.stride = sizeof(unsigned short);

  data.v.data = malloc(data.v.n * sizeof(YF_vterr));
  data.i.data = malloc(data.i.n * data.i.stride);
  if (data.v.data == NULL || data.i.data == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(data.v.data);
    free(data.i.data);
    return -1;
  }

  if (data.i.stride == sizeof(unsigned)) {
    unsigned *inds = data.i.data;
    unsigned idx = 0;
    unsigned k;
    for (unsigned i = 0; i < wid; ++i) {
      for (unsigned j = 0; j < dep; ++j) {
        k = (dep+1) * i + j;
        inds[idx++] = k;
        inds[idx++] = k+1;
        inds[idx++] = k+dep+2;
        inds[idx++] = k;
        inds[idx++] = k+dep+2;
        inds[idx++] = k+dep+1;
      }
    }
  } else {
    unsigned short *inds = data.i.data;
    unsigned idx = 0;
    unsigned k;
    for (unsigned i = 0; i < wid; ++i) {
      for (unsigned j = 0; j < dep; ++j) {
        k = (dep+1) * i + j;
        inds[idx++] = k;
        inds[idx++] = k+1;
        inds[idx++] = k+dep+2;
        inds[idx++] = k;
        inds[idx++] = k+dep+2;
        inds[idx++] = k+dep+1;
      }
    }
  }

  float x0, z0;
  float pos_offs;
  if (wid > dep) {
    x0 = -1.0f;
    z0 = (float)dep / (float)wid;
    pos_offs = 2.0f / (float)wid;
  } else {
    x0 = -(float)wid / (float)dep;
    z0 = 1.0f;
    pos_offs = 2.0f / (float)dep;
  }
  /* NxN textures are expected even for non-square grids */
  float tc_offs = pos_offs / 2.0f;

  float x, z, u, v;
  unsigned k;
  YF_vterr *vdt = data.v.data;
  for (unsigned i = 0; i <= wid; ++i) {
    x = x0 + pos_offs * (float)i;
    u = tc_offs * (float)i;
    for (unsigned j = 0; j <= dep; ++j) {
      z = z0 - pos_offs * (float)j;
      v = tc_offs * (float)(dep-j);
      k = (dep+1) * i + j;
      vdt[k].pos[0] = x;
      vdt[k].pos[1] = 0.0f;
      vdt[k].pos[2] = z;
      vdt[k].tc[0] = u;
      vdt[k].tc[1] = v;
      vdt[k].norm[0] = 0.0f;
      vdt[k].norm[1] = -1.0f;
      vdt[k].norm[2] = 0.0f;
    }
  }

  terr->mesh = yf_mesh_initdt(&data);
  free(data.v.data);
  free(data.i.data);
  if (terr->mesh == NULL)
    return -1;
  return 0;
}
