/*
 * YF
 * scene.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include <yf/com/yf-util.h>
#include <yf/com/yf-list.h>
#include <yf/com/yf-hashset.h>
#include <yf/com/yf-error.h>
#include <yf/core/yf-cmdbuf.h>

#include "scene.h"
#include "coreobj.h"
#include "resmgr.h"
#include "model.h"
#include "terrain.h"
#include "particle.h"
#include "quad.h"
#include "label.h"

#ifdef YF_DEVEL
# include <stdio.h>
# include "../test/print.h"
#endif

#ifdef YF_USE_FLOAT64
# define YF_CAMORIG (YF_vec3){0.0, 0.0, -10.0}
# define YF_CAMTGT  (YF_vec3){0.0, 0.0, 0.0}
# define YF_CAMASP  1.0
#else
# define YF_CAMORIG (YF_vec3){0.0f, 0.0f, -10.0f}
# define YF_CAMTGT  (YF_vec3){0.0f, 0.0f, 0.0f}
# define YF_CAMASP  1.0f
#endif

#undef YF_INSTCAP
#define YF_INSTCAP 4

#define YF_GLOBSZ      (sizeof(YF_mat4) << 1)
#define YF_INSTSZ_MDL  (sizeof(YF_mat4) << 1)
#define YF_INSTSZ_TERR (sizeof(YF_mat4) << 1)
#define YF_INSTSZ_PART (sizeof(YF_mat4) << 1)
#define YF_INSTSZ_QUAD (sizeof(YF_mat4) << 1)
#define YF_INSTSZ_LABL (sizeof(YF_mat4) << 1)

#define YF_PEND_NONE 0
#define YF_PEND_MDL  0x01
#define YF_PEND_MDLI 0x02
#define YF_PEND_TERR 0x04
#define YF_PEND_PART 0x08
#define YF_PEND_QUAD 0x10
#define YF_PEND_LABL 0x20

struct YF_scene_o {
  YF_node node;
  YF_camera cam;
  YF_color color;
  YF_viewport vport;
  YF_rect sciss;
};

/* Type defining shared variables available to all scenes. */
typedef struct {
  YF_context ctx;
  YF_buffer buf;
  unsigned buf_off;
  YF_list res_obtd;
  YF_cmdbuf cb;
  YF_hashset mdls;
  YF_hashset mdls_inst;
  YF_list terrs;
  YF_list parts;
  YF_list quads;
  YF_list labls;
} L_vars;

/* Type defining an entry in the list of obtained resources. */
typedef struct {
  int resrq;
  unsigned inst_alloc;
} L_reso;

/* Type representing key & value for use in the model sets. */
typedef struct {
  struct {
    YF_mesh mesh;
    YF_texture tex;
  } key;
  union {
    YF_model mdl;
    YF_model *mdls;
  };
  unsigned mdl_n;
  unsigned mdl_cap;
} L_kv_mdl;

/* Variables' instance. */
static L_vars l_vars = {0};

/* Initializes shared variables and prepares required resources. */
static int init_vars(void);

/* Traverses a scene graph to process its objects. */
static int traverse_scn(YF_node node, void *arg);

/* Renders model objects. */
static int render_mdl(YF_scene scn);

/* Renders model objects using instanced drawing. */
static int render_mdl_inst(YF_scene scn);

/* Renders terrain objects. */
static int render_terr(YF_scene scn);

/* Renders particle system objects. */
static int render_part(YF_scene scn);

/* Renders quad objects. */
static int render_quad(YF_scene scn);

/* Renders label objects. */
static int render_labl(YF_scene scn);

/* Copies uniform global data to buffer and updates dtable contents. */
static int copy_glob(YF_scene scn);

/* Copies uniform instance data to buffer and updates dtable contents. */
static int copy_inst(YF_scene scn, int resrq, void *objs, unsigned obj_n,
    YF_gstate gst, unsigned inst_alloc);

/* Yields all previously obtained resources. */
static void yield_res(void);

/* Clears data structures of all objects. */
static void clear_obj(void);

/* Functions used by the model sets. */
static size_t hash_mdl(const void *x);
static int cmp_mdl(const void *a, const void *b);
static int dealloc_mdl(void *val, void *arg);

YF_scene yf_scene_init(void) {
  if (l_vars.ctx == NULL && init_vars() != 0)
    return NULL;

  YF_scene scn = calloc(1, sizeof(struct YF_scene_o));
  if (scn == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  if ((scn->node = yf_node_init()) == NULL) {
    yf_scene_deinit(scn);
    return NULL;
  }
  if ((scn->cam = yf_camera_init(YF_CAMORIG, YF_CAMTGT, YF_CAMASP)) == NULL) {
    yf_scene_deinit(scn);
    return NULL;
  }
  scn->color = YF_COLOR_BLACK;

  return scn;
}

YF_node yf_scene_getnode(YF_scene scn) {
  assert(scn != NULL);
  return scn->node;
}

YF_camera yf_scene_getcam(YF_scene scn) {
  assert(scn != NULL);
  return scn->cam;
}

YF_color yf_scene_getcolor(YF_scene scn) {
  assert(scn != NULL);
  return scn->color;
}

void yf_scene_setcolor(YF_scene scn, YF_color color) {
  assert(scn != NULL);
  scn->color = color;
}

void yf_scene_deinit(YF_scene scn) {
  /* TODO: Deinitialize shared vars. on exit. */
  if (scn != NULL) {
    yf_camera_deinit(scn->cam);
    yf_node_deinit(scn->node);
    free(scn);
  }
}

int yf_scene_render(YF_scene scn, YF_pass pass, YF_target tgt, YF_dim2 dim) {
  assert(scn != NULL);
  assert(pass != NULL);
  assert(tgt != NULL);

  yf_camera_adjust(scn->cam, (YF_float)dim.width / (YF_float)dim.height);
  YF_VIEWPORT_FROMDIM2(dim, scn->vport);
  YF_VIEWPORT_SCISSOR(scn->vport, scn->sciss);

  int r = 0;
  yf_node_traverse(scn->node, traverse_scn, &r);
  if (r != 0) {
    clear_obj();
    return -1;
  }
  unsigned pend = YF_PEND_NONE;
  if (yf_hashset_getlen(l_vars.mdls) != 0)
    pend |= YF_PEND_MDL;
  if (yf_hashset_getlen(l_vars.mdls_inst) != 0)
    pend |= YF_PEND_MDLI;
  if (yf_list_getlen(l_vars.terrs) != 0)
    pend |= YF_PEND_TERR;
  if (yf_list_getlen(l_vars.parts) != 0)
    pend |= YF_PEND_PART;
  if (yf_list_getlen(l_vars.quads) != 0)
    pend |= YF_PEND_QUAD;
  if (yf_list_getlen(l_vars.labls) != 0)
    pend |= YF_PEND_LABL;

  l_vars.buf_off = 0;
  if ((l_vars.cb = yf_cmdbuf_get(l_vars.ctx, YF_CMDBUF_GRAPH)) == NULL) {
    clear_obj();
    return -1;
  }
  yf_cmdbuf_clearcolor(l_vars.cb, 0, scn->color);
  yf_cmdbuf_cleardepth(l_vars.cb, 1.0f);

  if (copy_glob(scn) != 0) {
    clear_obj();
    return -1;
  }
  yf_cmdbuf_setdtable(l_vars.cb, YF_RESIDX_GLOB, 0);

#ifdef YF_DEVEL
  unsigned exec_n = 0;
#endif

  do {
    yf_cmdbuf_settarget(l_vars.cb, tgt);
    yf_cmdbuf_setvport(l_vars.cb, 0, &scn->vport);
    yf_cmdbuf_setsciss(l_vars.cb, 0, scn->sciss);

    if (pend & YF_PEND_MDL) {
      if (render_mdl(scn) != 0) {
        yf_cmdbuf_end(l_vars.cb);
        yf_cmdbuf_reset(l_vars.ctx);
        yield_res();
        clear_obj();
        return -1;
      }
      if (yf_hashset_getlen(l_vars.mdls) == 0)
        pend &= ~YF_PEND_MDL;
    }

    if (pend & YF_PEND_MDLI) {
      if (render_mdl_inst(scn) != 0) {
        yf_cmdbuf_end(l_vars.cb);
        yf_cmdbuf_reset(l_vars.ctx);
        yield_res();
        clear_obj();
        return -1;
      }
      if (yf_hashset_getlen(l_vars.mdls_inst) == 0)
        pend &= ~YF_PEND_MDLI;
    }

    if (pend & YF_PEND_TERR) {
      if (render_terr(scn) != 0) {
        yf_cmdbuf_end(l_vars.cb);
        yf_cmdbuf_reset(l_vars.ctx);
        yield_res();
        clear_obj();
        return -1;
      }
      if (yf_list_getlen(l_vars.terrs) == 0)
        pend &= ~YF_PEND_TERR;
    }

    if (pend & YF_PEND_PART) {
      if (render_part(scn) != 0) {
        yf_cmdbuf_end(l_vars.cb);
        yf_cmdbuf_reset(l_vars.ctx);
        yield_res();
        clear_obj();
        return -1;
      }
      if (yf_list_getlen(l_vars.parts) == 0)
        pend &= ~YF_PEND_PART;
    }

    if (pend & YF_PEND_QUAD) {
      if (render_quad(scn) != 0) {
        yf_cmdbuf_end(l_vars.cb);
        yf_cmdbuf_reset(l_vars.ctx);
        yield_res();
        clear_obj();
        return -1;
      }
      if (yf_list_getlen(l_vars.quads) == 0)
        pend &= ~YF_PEND_QUAD;
    }

    if (pend & YF_PEND_LABL) {
      if (render_labl(scn) != 0) {
        yf_cmdbuf_end(l_vars.cb);
        yf_cmdbuf_reset(l_vars.ctx);
        yield_res();
        clear_obj();
        return -1;
      }
      if (yf_list_getlen(l_vars.labls) == 0)
        pend &= ~YF_PEND_LABL;
    }

    if (yf_cmdbuf_end(l_vars.cb) == 0) {
      if (yf_cmdbuf_exec(l_vars.ctx) != 0) {
        yield_res();
        clear_obj();
        return -1;
      }
    } else {
      yf_cmdbuf_reset(l_vars.ctx);
      yield_res();
      clear_obj();
      return -1;
    }

#ifdef YF_DEVEL
    ++exec_n;
#endif

    yield_res();

    if (pend != YF_PEND_NONE) {
      if ((l_vars.cb = yf_cmdbuf_get(l_vars.ctx, YF_CMDBUF_GRAPH)) == NULL) {
        clear_obj();
        return -1;
      }
    } else {
      break;
    }
  } while (1);

#ifdef YF_DEVEL
  printf("\n[YF] OUTPUT (%s):\n number of executions: %u\n\n",
      __func__, exec_n);
#endif

  clear_obj();
  return 0;
}

static int init_vars(void) {
  assert(l_vars.ctx == NULL);

  if ((l_vars.ctx = yf_getctx()) == NULL)
    return -1;

  /* TODO: Check limits. */
  unsigned insts[YF_RESRQ_N] = {
    [YF_RESRQ_MDL]   = 128,
    [YF_RESRQ_MDL4]  = 48,
    [YF_RESRQ_MDL16] = 48,
    [YF_RESRQ_MDL64] = 16,
    [YF_RESRQ_TERR]  = 16,
    [YF_RESRQ_PART]  = 24,
    [YF_RESRQ_QUAD]  = 48,
    [YF_RESRQ_LABL]  = 64
  };
  size_t inst_min = 0;
  size_t inst_sum = 0;
  for (unsigned i = 0; i < YF_RESRQ_N; ++i) {
    inst_min += insts[i] != 0;
    inst_sum += insts[i];
  }
  assert(inst_min > 0);
  size_t buf_sz;

  do {
    int failed = 0;
    buf_sz = YF_GLOBSZ;
    for (unsigned i = 0; i < YF_RESRQ_N; ++i) {
      if (yf_resmgr_setallocn(i, insts[i]) != 0 || yf_resmgr_prealloc(i) != 0) {
        yf_resmgr_clear();
        failed = 1;
        break;
      }
      switch (i) {
        case YF_RESRQ_MDL:
          buf_sz += insts[i] * YF_INSTSZ_MDL;
          break;
        case YF_RESRQ_MDL4:
          buf_sz += insts[i] * (YF_INSTSZ_MDL<<2);
          break;
        case YF_RESRQ_MDL16:
          buf_sz += insts[i] * (YF_INSTSZ_MDL<<4);
          break;
        case YF_RESRQ_MDL64:
          buf_sz += insts[i] * (YF_INSTSZ_MDL<<6);
          break;
        case YF_RESRQ_TERR:
          buf_sz += insts[i] * YF_INSTSZ_TERR;
          break;
        case YF_RESRQ_PART:
          buf_sz += insts[i] * YF_INSTSZ_PART;
          break;
        case YF_RESRQ_QUAD:
          buf_sz += insts[i] * YF_INSTSZ_QUAD;
          break;
        case YF_RESRQ_LABL:
          buf_sz += insts[i] * YF_INSTSZ_LABL;
          break;
        default:
          assert(0);
      }
    }
    /* proceed if all allocations succeed */
    if (!failed)
      break;
    /* give up if cannot allocate the minimum */
    if (inst_sum <= inst_min)
      return -1;
    /* try again with reduced number of instances */
    inst_sum = 0;
    for (unsigned i = 0; i < YF_RESRQ_N; ++i) {
      if (insts[i] > 0) {
        insts[i] = YF_MAX(1, insts[i] >> 1);
        inst_sum += insts[i];
      }
    }
  } while (1);

  if ((l_vars.buf = yf_buffer_init(l_vars.ctx, buf_sz)) == NULL ||
      (l_vars.res_obtd = yf_list_init(NULL)) == NULL ||
      (l_vars.mdls = yf_hashset_init(hash_mdl, cmp_mdl)) == NULL ||
      (l_vars.mdls_inst = yf_hashset_init(hash_mdl, cmp_mdl)) == NULL ||
      (l_vars.terrs = yf_list_init(NULL)) == NULL ||
      (l_vars.parts = yf_list_init(NULL)) == NULL ||
      (l_vars.quads = yf_list_init(NULL)) == NULL ||
      (l_vars.labls = yf_list_init(NULL)) == NULL)
  {
    yf_resmgr_clear();
    yf_buffer_deinit(l_vars.buf);
    yf_list_deinit(l_vars.res_obtd);
    yf_hashset_deinit(l_vars.mdls);
    yf_hashset_deinit(l_vars.mdls_inst);
    yf_list_deinit(l_vars.terrs);
    yf_list_deinit(l_vars.parts);
    yf_list_deinit(l_vars.quads);
    yf_list_deinit(l_vars.labls);
    memset(&l_vars, 0, sizeof l_vars);
    return -1;
  }
  return 0;
}

static int traverse_scn(YF_node node, void *arg) {
  void *obj = NULL;
  const int nodeobj = yf_node_getobj(node, &obj);

  switch (nodeobj) {
    case YF_NODEOBJ_MODEL: {
      YF_model mdl = obj;
      L_kv_mdl key = {
        {yf_model_getmesh(mdl), yf_model_gettex(mdl)},
        {NULL}, 0, 0
      };
      L_kv_mdl *val = NULL;
      if ((val = yf_hashset_search(l_vars.mdls, &key)) != NULL) {
        /* model with shared resources, move to instanced drawing set */
        YF_model *mdls = malloc(YF_INSTCAP * sizeof mdl);
        if (mdls == NULL) {
          yf_seterr(YF_ERR_NOMEM, __func__);
          *(int *)arg = -1;
          return -1;
        }
        yf_hashset_remove(l_vars.mdls, val);
        if (yf_hashset_insert(l_vars.mdls_inst, val) != 0) {
          free(mdls);
          *(int *)arg = -1;
          return -1;
        }
        mdls[0] = val->mdl;
        mdls[1] = mdl;
        val->mdls = mdls;
        val->mdl_n = 2;
        val->mdl_cap = YF_INSTCAP;
      } else if ((val = yf_hashset_search(l_vars.mdls_inst, &key)) != NULL) {
        /* another model for instanced drawing */
        if (val->mdl_n == val->mdl_cap) {
          YF_model *mdls = realloc(val->mdls, (val->mdl_cap * sizeof mdl) << 1);
          if (mdls == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            *(int *)arg = -1;
            return -1;
          }
          val->mdls = mdls;
          val->mdl_cap *= 2;
        }
        val->mdls[val->mdl_n++] = mdl;
      } else {
        /* new unique model */
        if ((val = malloc(sizeof *val)) == NULL) {
          yf_seterr(YF_ERR_NOMEM, __func__);
          *(int *)arg = -1;
          return -1;
        }
        *val = key;
        if (yf_hashset_insert(l_vars.mdls, val) != 0) {
          *(int *)arg = -1;
          return -1;
        }
        val->mdl = mdl;
        val->mdl_n = 1;
        val->mdl_cap = 1;
      }
    } break;

    case YF_NODEOBJ_TERRAIN:
      if (yf_list_insert(l_vars.terrs, obj) != 0) {
        *(int *)arg = -1;
        return -1;
      }
      break;

    case YF_NODEOBJ_PARTICLE:
      if (yf_list_insert(l_vars.parts, obj) != 0) {
        *(int *)arg = -1;
        return -1;
      }
      break;

    case YF_NODEOBJ_QUAD:
      if (yf_list_insert(l_vars.quads, obj) != 0) {
        *(int *)arg = -1;
        return -1;
      }
      break;

    case YF_NODEOBJ_LABEL:
      if (yf_list_insert(l_vars.labls, obj) != 0) {
        *(int *)arg = -1;
        return -1;
      }
      break;

    case YF_NODEOBJ_LIGHT:
      /* TODO */
      assert(0);

    case YF_NODEOBJ_EFFECT:
      /* TODO */
      assert(0);

    default:
      /* this is not a drawable object, nothing to do */
      break;
  }

#ifdef YF_DEVEL
  yf_print_nodeobj(node);
#endif
  return 0;
}

static int render_mdl(YF_scene scn) {
  YF_gstate gst = NULL;
  unsigned inst_alloc = 0;
  YF_dtable inst_dtb = NULL;
  L_reso *reso = NULL;
  YF_texture tex = NULL;
  YF_mesh mesh = NULL;
  YF_iter it = YF_NILIT;
  L_kv_mdl *val = NULL;

  do {
    val = yf_hashset_next(l_vars.mdls, &it);
    if (YF_IT_ISNIL(it))
      break;

    if ((gst = yf_resmgr_obtain(YF_RESRQ_MDL, &inst_alloc)) == NULL) {
      switch (yf_geterr()) {
        case YF_ERR_INUSE:
          /* out of resources, need to execute pending work */
          return 0;
        default:
          return -1;
      }
    }
    inst_dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);

    if ((reso = malloc(sizeof *reso)) == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    reso->resrq = YF_RESRQ_MDL;
    reso->inst_alloc = inst_alloc;
    if (yf_list_insert(l_vars.res_obtd, reso) != 0) {
      free(reso);
      return -1;
    }

    if (copy_inst(scn, YF_RESRQ_MDL, &val->mdl, 1, gst, inst_alloc) != 0)
      return -1;

    if ((tex = yf_model_gettex(val->mdl)) != NULL)
      yf_texture_copyres(tex, inst_dtb, inst_alloc, YF_RESBIND_TEX, 0);
    else
      /* TODO: Handle models lacking texture. */
      assert(0);

    yf_cmdbuf_setgstate(l_vars.cb, gst);
    yf_cmdbuf_setdtable(l_vars.cb, YF_RESIDX_INST, inst_alloc);

    if ((mesh = yf_model_getmesh(val->mdl)) != NULL)
      yf_mesh_draw(mesh, l_vars.cb, 1, 0);
    else
      /* TODO: Handle models lacking mesh. */
      assert(0);

    yf_hashset_remove(l_vars.mdls, val);
    it = YF_NILIT;
  } while (1);

  return 0;
}

static int render_mdl_inst(YF_scene scn) {
  static const int resrq[] = {YF_RESRQ_MDL4, YF_RESRQ_MDL16, YF_RESRQ_MDL64};
  static const unsigned insts[] = {4, 16, 64};
  static const int sz = sizeof resrq / sizeof resrq[0];

  unsigned n, rem;
  int rq_i;

  YF_gstate gst = NULL;
  unsigned inst_alloc = 0;
  YF_dtable inst_dtb = NULL;
  L_reso *reso = NULL;
  YF_texture tex = NULL;
  YF_mesh mesh = NULL;
  YF_iter it = YF_NILIT;
  L_kv_mdl *val = NULL;
  YF_list done = yf_list_init(NULL);
  if (done == NULL)
    return -1;

  do {
    val = yf_hashset_next(l_vars.mdls_inst, &it);
    if (YF_IT_ISNIL(it))
      break;

    rem = val->mdl_n;
    n = 0;

    do {
      rq_i = 0;
      for (; rq_i < sz; ++rq_i) {
        if (insts[rq_i] >= rem)
          break;
      }
      rq_i = YF_MIN(rq_i, sz-1);

      for (int i = rq_i; i >= 0; --i) {
        if ((gst = yf_resmgr_obtain(resrq[i], &inst_alloc)) != NULL) {
          n = YF_MIN(insts[i], rem);
          rem -= n;
          rq_i = i;
          break;
        }
      }
      if (gst == NULL) {
        if (yf_geterr() == YF_ERR_INUSE) {
          /* out of resources, the remaining instances for this entry
             will be rendered in future passes */
          val->mdl_n = rem;
          break;
        } else {
          yf_list_deinit(done);
          return -1;
        }
      }
      inst_dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);

      if ((reso = malloc(sizeof *reso)) == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_list_deinit(done);
        return -1;
      }
      reso->resrq = resrq[rq_i];
      reso->inst_alloc = inst_alloc;
      if (yf_list_insert(l_vars.res_obtd, reso) != 0) {
        free(reso);
        yf_list_deinit(done);
        return -1;
      }

      if (copy_inst(scn, resrq[rq_i], val->mdls+rem, n, gst, inst_alloc) != 0) {
        yf_list_deinit(done);
        return -1;
      }

      if ((tex = yf_model_gettex(val->mdls[rem])) != NULL)
        yf_texture_copyres(tex, inst_dtb, inst_alloc, YF_RESBIND_TEX, 0);
      else
        /* TODO: Handle models lacking texture. */
        assert(0);

      yf_cmdbuf_setgstate(l_vars.cb, gst);
      yf_cmdbuf_setdtable(l_vars.cb, YF_RESIDX_INST, inst_alloc);

      if ((mesh = yf_model_getmesh(val->mdls[rem])) != NULL)
        yf_mesh_draw(mesh, l_vars.cb, n, 0);
      else
        /* TODO: Handle models lacking mesh. */
        assert(0);

      if (rem == 0) {
        /* cannot invalidate the set iterator */
        if (yf_list_insert(done, val) == 0)
          break;
        yf_list_deinit(done);
        return -1;
      }
    } while (1);

  } while (1);

  while ((val = yf_list_removeat(done, NULL)) != NULL)
    yf_hashset_remove(l_vars.mdls_inst, val);
  yf_list_deinit(done);

  return 0;
}

static int render_terr(YF_scene scn) {
  YF_gstate gst = NULL;
  unsigned inst_alloc = 0;
  YF_dtable inst_dtb = NULL;
  L_reso *reso = NULL;
  YF_texture hmap = NULL;
  YF_texture tex = NULL;
  YF_mesh mesh = NULL;
  YF_iter it = YF_NILIT;
  YF_terrain terr = NULL;

  do {
    terr = yf_list_next(l_vars.terrs, &it);
    if (YF_IT_ISNIL(it))
      break;

    if ((gst = yf_resmgr_obtain(YF_RESRQ_TERR, &inst_alloc)) == NULL) {
      switch (yf_geterr()) {
        case YF_ERR_INUSE:
          /* out of resources, need to execute pending work */
          return 0;
        default:
          return -1;
      }
    }
    inst_dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);

    if ((reso = malloc(sizeof *reso)) == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    reso->resrq = YF_RESRQ_TERR;
    reso->inst_alloc = inst_alloc;
    if (yf_list_insert(l_vars.res_obtd, reso) != 0) {
      free(reso);
      return -1;
    }

    if (copy_inst(scn, YF_RESRQ_TERR, &terr, 1, gst, inst_alloc) != 0)
      return -1;

    if ((hmap = yf_terrain_gethmap(terr)) != NULL)
      yf_texture_copyres(hmap, inst_dtb, inst_alloc, YF_RESBIND_HMAP, 0);
    else
      /* TODO: Handle terrains lacking height map. */
      assert(0);

    if ((tex = yf_terrain_gettex(terr)) != NULL)
      yf_texture_copyres(tex, inst_dtb, inst_alloc, YF_RESBIND_TEX, 0);
    else
      /* TODO: Handle terrains lacking texture. */
      assert(0);

    yf_cmdbuf_setgstate(l_vars.cb, gst);
    yf_cmdbuf_setdtable(l_vars.cb, YF_RESIDX_INST, inst_alloc);

    mesh = yf_terrain_getmesh(terr);
    yf_mesh_draw(mesh, l_vars.cb, 1, 0);

    yf_list_removeat(l_vars.terrs, &it);
    it = YF_NILIT;
  } while (1);

  return 0;
}

static int render_part(YF_scene scn) {
  YF_gstate gst = NULL;
  unsigned inst_alloc = 0;
  YF_dtable inst_dtb = NULL;
  L_reso *reso = NULL;
  YF_texture tex = NULL;
  YF_mesh mesh = NULL;
  YF_iter it = YF_NILIT;
  YF_particle part = NULL;

  do {
    part = yf_list_next(l_vars.parts, &it);
    if (YF_IT_ISNIL(it))
      break;

    if ((gst = yf_resmgr_obtain(YF_RESRQ_PART, &inst_alloc)) == NULL) {
      switch (yf_geterr()) {
        case YF_ERR_INUSE:
          /* out of resources, need to execute pending work */
          return 0;
        default:
          return -1;
      }
    }
    inst_dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);

    if ((reso = malloc(sizeof *reso)) == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    reso->resrq = YF_RESRQ_PART;
    reso->inst_alloc = inst_alloc;
    if (yf_list_insert(l_vars.res_obtd, reso) != 0) {
      free(reso);
      return -1;
    }

    if (copy_inst(scn, YF_RESRQ_PART, &part, 1, gst, inst_alloc) != 0)
      return -1;

    if ((tex = yf_particle_gettex(part)) != NULL)
      yf_texture_copyres(tex, inst_dtb, inst_alloc, YF_RESBIND_TEX, 0);
    else
      /* TODO: Handle particle systems lacking texture. */
      assert(0);

    yf_cmdbuf_setgstate(l_vars.cb, gst);
    yf_cmdbuf_setdtable(l_vars.cb, YF_RESIDX_INST, inst_alloc);

    mesh = yf_particle_getmesh(part);
    yf_mesh_draw(mesh, l_vars.cb, 1, 0);

    yf_list_removeat(l_vars.parts, &it);
    it = YF_NILIT;
  } while (1);

  return 0;
}

static int render_quad(YF_scene scn) {
  YF_gstate gst = NULL;
  unsigned inst_alloc = 0;
  YF_dtable inst_dtb = NULL;
  L_reso* reso = NULL;
  YF_texture tex = NULL;
  YF_mesh mesh = NULL;
  YF_iter it = YF_NILIT;
  YF_quad quad = NULL;

  do {
    quad = yf_list_next(l_vars.quads, &it);
    if (YF_IT_ISNIL(it))
      break;

    if ((gst = yf_resmgr_obtain(YF_RESRQ_QUAD, &inst_alloc)) == NULL) {
      switch (yf_geterr()) {
        case YF_ERR_INUSE:
          /* out of resources, need to execute pending work */
          return 0;
        default:
          return -1;
      }
    }
    inst_dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);

    if ((reso = malloc(sizeof *reso)) == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    reso->resrq = YF_RESRQ_QUAD;
    reso->inst_alloc = inst_alloc;
    if (yf_list_insert(l_vars.res_obtd, reso) != 0) {
      free(reso);
      return -1;
    }

    if (copy_inst(scn, YF_RESRQ_QUAD, &quad, 1, gst, inst_alloc) != 0)
      return -1;

    if ((tex = yf_quad_gettex(quad)) != NULL)
      yf_texture_copyres(tex, inst_dtb, inst_alloc, YF_RESBIND_TEX, 0);
    else
      /* TODO: Handle quads lacking texture. */
      assert(0);

    yf_cmdbuf_setgstate(l_vars.cb, gst);
    yf_cmdbuf_setdtable(l_vars.cb, YF_RESIDX_INST, inst_alloc);

    mesh = yf_quad_getmesh(quad);
    yf_mesh_draw(mesh, l_vars.cb, 1, 0);

    yf_list_removeat(l_vars.quads, &it);
    it = YF_NILIT;
  } while (1);

  return 0;
}

static int render_labl(YF_scene scn) {
  YF_gstate gst = NULL;
  unsigned inst_alloc = 0;
  YF_dtable inst_dtb = NULL;
  L_reso *reso = NULL;
  YF_texture tex = NULL;
  YF_mesh mesh = NULL;
  YF_iter it = YF_NILIT;
  YF_label labl = NULL;

  do {
    labl = yf_list_next(l_vars.labls, &it);
    if (YF_IT_ISNIL(it))
      break;

    if ((gst = yf_resmgr_obtain(YF_RESRQ_LABL, &inst_alloc)) == NULL) {
      switch (yf_geterr()) {
        case YF_ERR_INUSE:
          /* out of resources, need to execute pending work */
          return 0;
        default:
          return -1;
      }
    }
    inst_dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);

    if ((reso = malloc(sizeof *reso)) == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    reso->resrq = YF_RESRQ_LABL;
    reso->inst_alloc = inst_alloc;
    if (yf_list_insert(l_vars.res_obtd, reso) != 0) {
      free(reso);
      return -1;
    }

    if (copy_inst(scn, YF_RESRQ_LABL, &labl, 1, gst, inst_alloc) != 0)
      return -1;

    /* FIXME: Texture may be invalid. */
    tex = yf_label_gettex(labl);
    yf_texture_copyres(tex, inst_dtb, inst_alloc, YF_RESBIND_TEX, 0);

    yf_cmdbuf_setgstate(l_vars.cb, gst);
    yf_cmdbuf_setdtable(l_vars.cb, YF_RESIDX_INST, inst_alloc);

    mesh = yf_label_getmesh(labl);
    yf_mesh_draw(mesh, l_vars.cb, 1, 0);

    yf_list_removeat(l_vars.labls, &it);
    it = YF_NILIT;
  } while (1);

  return 0;
}

static int copy_glob(YF_scene scn) {
  YF_dtable dtb = yf_resmgr_getglob();
  if (dtb == NULL)
    return -1;

  const YF_slice elems = {0, 1};
  size_t off = l_vars.buf_off;
  size_t sz = YF_GLOBSZ;

  /* view matrix */
  if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
        *yf_camera_getview(scn->cam), sizeof(YF_mat4)) != 0)
    return -1;
  l_vars.buf_off += sizeof(YF_mat4);

  /* projection matrix */
  if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
        *yf_camera_getproj(scn->cam), sizeof(YF_mat4)) != 0)
    return -1;
  l_vars.buf_off += sizeof(YF_mat4);

  /* copy */
  if (yf_dtable_copybuf(dtb, 0, YF_RESBIND_GLOB, elems,
        &l_vars.buf, &off, &sz) != 0)
    return -1;

  return 0;
}

static int copy_inst(YF_scene scn, int resrq, void *objs, unsigned obj_n,
    YF_gstate gst, unsigned inst_alloc)
{
  YF_dtable dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);
  const YF_slice elems = {0, 1};
  size_t off, sz;

  switch (resrq) {
    case YF_RESRQ_MDL:
    case YF_RESRQ_MDL4:
    case YF_RESRQ_MDL16:
    case YF_RESRQ_MDL64:
      off = l_vars.buf_off;
      sz = obj_n * YF_INSTSZ_MDL;
      for (unsigned i = 0; i < obj_n; ++i) {
        YF_model mdl = ((YF_model *)objs)[i];
        yf_mat4_mul(*yf_model_getmvp(mdl), *yf_camera_getxform(scn->cam),
            *yf_model_getxform(mdl));
        /* model matrix */
        if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
              *yf_model_getxform(mdl), sizeof(YF_mat4)) != 0)
          return -1;
        l_vars.buf_off += sizeof(YF_mat4);
        /* model-view-projection matrix */
        if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
              *yf_model_getmvp(mdl), sizeof(YF_mat4)) != 0)
          return -1;
        l_vars.buf_off += sizeof(YF_mat4);
      }
      /* copy */
      if (yf_dtable_copybuf(dtb, inst_alloc, YF_RESBIND_INST, elems,
            &l_vars.buf, &off, &sz) != 0)
        return -1;
      break;

    case YF_RESRQ_TERR:
      assert(obj_n == 1);
      off = l_vars.buf_off;
      sz = obj_n * YF_INSTSZ_TERR;
      {
        YF_terrain terr = ((YF_terrain *)objs)[0];
        yf_mat4_mul(*yf_terrain_getmvp(terr), *yf_camera_getxform(scn->cam),
            *yf_terrain_getxform(terr));
        /* model matrix */
        if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
              *yf_terrain_getxform(terr), sizeof(YF_mat4)) != 0)
          return -1;
        l_vars.buf_off += sizeof(YF_mat4);
        /* model-view-projection matrix */
        if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
              *yf_terrain_getmvp(terr), sizeof(YF_mat4)) != 0)
          return -1;
        l_vars.buf_off += sizeof(YF_mat4);
        /* copy */
        if (yf_dtable_copybuf(dtb, inst_alloc, YF_RESBIND_INST, elems,
              &l_vars.buf, &off, &sz) != 0)
          return -1;
      }
      break;

    case YF_RESRQ_PART:
      assert(obj_n == 1);
      off = l_vars.buf_off;
      sz = obj_n * YF_INSTSZ_PART;
      {
        YF_particle part = ((YF_particle *)objs)[0];
        yf_mat4_mul(*yf_particle_getmvp(part), *yf_camera_getxform(scn->cam),
            *yf_particle_getxform(part));
        /* model matrix */
        if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
              *yf_particle_getxform(part), sizeof(YF_mat4)) != 0)
          return -1;
        l_vars.buf_off += sizeof(YF_mat4);
        /* model-view-projection matrix */
        if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
              *yf_particle_getmvp(part), sizeof(YF_mat4)) != 0)
          return -1;
        l_vars.buf_off += sizeof(YF_mat4);
        /* copy */
        if (yf_dtable_copybuf(dtb, inst_alloc, YF_RESBIND_INST, elems,
              &l_vars.buf, &off, &sz) != 0)
          return -1;
      }
      break;

    case YF_RESRQ_QUAD:
      assert(obj_n == 1);
      off = l_vars.buf_off;
      sz = obj_n * YF_INSTSZ_QUAD;
      {
        YF_quad quad = ((YF_quad *)objs)[0];
        yf_mat4_mul(*yf_quad_getmvp(quad), *yf_camera_getxform(scn->cam),
            *yf_quad_getxform(quad));
        /* model matrix */
        if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
              *yf_quad_getxform(quad), sizeof(YF_mat4)) != 0)
          return -1;
        l_vars.buf_off += sizeof(YF_mat4);
        /* model-view-projection matrix */
        if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
              *yf_quad_getmvp(quad), sizeof(YF_mat4)) != 0)
          return -1;
        l_vars.buf_off += sizeof(YF_mat4);
        /* copy */
        if (yf_dtable_copybuf(dtb, inst_alloc, YF_RESBIND_INST, elems,
              &l_vars.buf, &off, &sz) != 0)
          return -1;
      }
      break;

    case YF_RESRQ_LABL:
      assert(obj_n == 1);
      off = l_vars.buf_off;
      sz = obj_n * YF_INSTSZ_LABL;
      {
        YF_label labl = ((YF_label *)objs)[0];
        yf_mat4_mul(*yf_label_getmvp(labl), *yf_camera_getxform(scn->cam),
            *yf_label_getxform(labl));
        /* model matrix */
        if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
              *yf_label_getxform(labl), sizeof(YF_mat4)) != 0)
          return -1;
        l_vars.buf_off += sizeof(YF_mat4);
        /* model-view-projection matrix */
        if (yf_buffer_copy(l_vars.buf, l_vars.buf_off,
              *yf_label_getmvp(labl), sizeof(YF_mat4)) != 0)
          return -1;
        l_vars.buf_off += sizeof(YF_mat4);
        /* copy */
        if (yf_dtable_copybuf(dtb, inst_alloc, YF_RESBIND_INST, elems,
              &l_vars.buf, &off, &sz) != 0)
          return -1;
      }
      break;

    default:
      assert(0);
  }

  return 0;
}

static void yield_res(void) {
  L_reso *val;
  while ((val = yf_list_removeat(l_vars.res_obtd, NULL)) != NULL) {
    yf_resmgr_yield(val->resrq, val->inst_alloc);
    free(val);
  }
}

static void clear_obj(void) {
  if (yf_hashset_getlen(l_vars.mdls) != 0) {
    yf_hashset_each(l_vars.mdls, dealloc_mdl, NULL);
    yf_hashset_clear(l_vars.mdls);
  }
  if (yf_hashset_getlen(l_vars.mdls_inst) != 0) {
    yf_hashset_each(l_vars.mdls_inst, dealloc_mdl, NULL);
    yf_hashset_clear(l_vars.mdls_inst);
  }
  yf_list_clear(l_vars.terrs);
  yf_list_clear(l_vars.parts);
  yf_list_clear(l_vars.quads);
  yf_list_clear(l_vars.labls);
}

static size_t hash_mdl(const void *x) {
  const L_kv_mdl *kv = x;
  return (size_t)kv->key.mesh ^ (size_t)kv->key.tex ^ 0x516536655d2b;
}

static int cmp_mdl(const void *a, const void *b) {
  const L_kv_mdl *kv1 = a;
  const L_kv_mdl *kv2 = b;
  return !(kv1->key.mesh == kv2->key.mesh && kv1->key.tex == kv2->key.tex);
}

static int dealloc_mdl(void *val, YF_UNUSED void *arg) {
  L_kv_mdl *kv = val;
  if (kv->mdl_n > 1)
    free(kv->mdls);
  free(val);
  return 0;
}
