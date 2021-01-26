/*
 * YF
 * resmgr.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "resmgr.h"
#include "coreobj.h"
#include "node.h"
#include "vertex.h"

#ifndef YF_SHD_DIR
# define YF_SHD_DIR "bin/"
#endif
#ifndef YF_SHD_FILEEXT
# define YF_SHD_FILEEXT ".spv"
#endif

/* XXX: This arbitrary defaults are expected to be replaced by 'setallocn'. */
#define YF_ALLOCN_MDL   64
#define YF_ALLOCN_MDL4  16
#define YF_ALLOCN_MDL16 16
#define YF_ALLOCN_MDL64 4
#define YF_ALLOCN_TERR  8
#define YF_ALLOCN_PART  32

/* Type defining an entry in the resource list. */
typedef struct {
  YF_gstate gst;
  char *obtained;
  unsigned n;
  unsigned i;
} L_entry;

/* List of resources, indexed by 'resrq' values. */
static L_entry l_entries[YF_RESRQ_N] = {0};

/* Global dtable. */
static YF_dtable l_glob = NULL;

/* Sizes used for instance allocations, indexed by 'resrq' values. */
static unsigned l_allocn[YF_RESRQ_N] = {
  [YF_RESRQ_MDL]   = YF_ALLOCN_MDL,
  [YF_RESRQ_MDL4]  = YF_ALLOCN_MDL4,
  [YF_RESRQ_MDL16] = YF_ALLOCN_MDL16,
  [YF_RESRQ_MDL64] = YF_ALLOCN_MDL64,
  [YF_RESRQ_TERR]  = YF_ALLOCN_TERR,
  [YF_RESRQ_PART]  = YF_ALLOCN_PART
};

/* Initializes the entry of a given 'resrq' value. */
static int init_entry(int resrq);

/* Deinitializes the entry of a given 'resrq' value. */
static void deinit_entry(int resrq);

/* Initializes the entry of a model resource. */
static int init_mdl(L_entry *entry, unsigned elements);

/* Initializes the entry of a terrain resource. */
static int init_terr(L_entry *entry);

/* Initializes the entry of a particle system resource. */
static int init_part(L_entry *entry);

/* Makes a string to use as the pathname of a shader module.
   The caller is responsible for deallocating the returned string. */
static char *make_shdpath(int nodeobj, int stage, unsigned elements);

YF_gstate yf_resmgr_obtain(int resrq, unsigned *inst_alloc) {
  assert(resrq >= 0 && resrq < YF_RESRQ_N);
  assert(inst_alloc != NULL);

  if (l_allocn[resrq] == 0) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return NULL;
  }

  YF_gstate gst = NULL;
  if (l_entries[resrq].gst == NULL) {
    if (init_entry(resrq) == 0) {
      gst = l_entries[resrq].gst;
      *inst_alloc = 0;
      l_entries[resrq].obtained[0] = 1;
      l_entries[resrq].i = 1 % l_entries[resrq].n;
    }
  } else {
    unsigned j;
    for (unsigned i = 0; i < l_entries[resrq].n; ++i) {
      j = (l_entries[resrq].i + i) % l_entries[resrq].n;
      if (!l_entries[resrq].obtained[j]) {
        gst = l_entries[resrq].gst;
        *inst_alloc = j;
        l_entries[resrq].obtained[j] = 1;
        l_entries[resrq].i = (j + 1) % l_entries[resrq].n;
        break;
      }
    }
    if (gst == NULL)
      yf_seterr(YF_ERR_INUSE, __func__);
  }
  return gst;
}

void yf_resmgr_yield(int resrq, unsigned inst_alloc) {
  assert(resrq >= 0 && resrq < YF_RESRQ_N);
  assert(inst_alloc < l_entries[resrq].n);

  l_entries[resrq].obtained[inst_alloc] = 0;
  l_entries[resrq].i = inst_alloc;
}

YF_dtable yf_resmgr_getglob(void) {
  if (l_glob != NULL)
    return l_glob;

  YF_context ctx = yf_getctx();
  assert(ctx != NULL);

  const YF_dentry ents[] = {{YF_RESIDX_GLOB, YF_DTYPE_UNIFORM, 1, NULL}};
  l_glob = yf_dtable_init(ctx, ents, sizeof ents / sizeof ents[0]);
  if (l_glob == NULL || yf_dtable_alloc(l_glob, 1) != 0) {
    yf_dtable_deinit(l_glob);
    l_glob = NULL;
  }
  return l_glob;
}

unsigned yf_resmgr_getallocn(int resrq) {
  assert(resrq >= 0 && resrq < YF_RESRQ_N);
  return l_entries[resrq].n;
}

int yf_resmgr_setallocn(int resrq, unsigned n) {
  assert(resrq >= 0 && resrq < YF_RESRQ_N);

  if (n == l_allocn[resrq])
    return 0;

  if (l_entries[resrq].gst != NULL) {
    YF_dtable dtb = yf_gstate_getdtb(l_entries[resrq].gst, YF_RESIDX_INST);
    if (yf_dtable_alloc(dtb, n) != 0) {
      l_entries[resrq].n = 0;
      return -1;
    }
    if (n > 0) {
      char *tmp = realloc(l_entries[resrq].obtained, n);
      if (tmp == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_dtable_dealloc(dtb);
        l_entries[resrq].n = 0;
        return -1;
      }
      memset(tmp, 0, n);
      l_entries[resrq].obtained = tmp;
    } else {
      /* XXX: The state object and global resource are intentionally left
         untouched here. */
      free(l_entries[resrq].obtained);
      l_entries[resrq].obtained = NULL;
    }
    l_entries[resrq].n = n;
    l_entries[resrq].i = 0;
  }

  l_allocn[resrq] = n;
  return 0;
}

int yf_resmgr_prealloc(int resrq) {
  assert(resrq >= 0 && resrq < YF_RESRQ_N);

  if (l_allocn[resrq] == 0)
    return 0;

  if (l_entries[resrq].gst != NULL) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  return init_entry(resrq);
}

void yf_resmgr_dealloc(int resrq) {
  assert(resrq >= 0 && resrq < YF_RESRQ_N);

  if (l_entries[resrq].gst != NULL)
    deinit_entry(resrq);
}

void yf_resmgr_clear(void) {
  for (unsigned i = 0; i < YF_RESRQ_N; ++i) {
    if (l_entries[i].gst != NULL)
      deinit_entry(i);
  }
  yf_dtable_deinit(l_glob);
  l_glob = NULL;
}

static int init_entry(int resrq) {
  assert(resrq >= 0 && resrq < YF_RESRQ_N);
  assert(l_allocn[resrq] > 0);

  if (yf_resmgr_getglob() == NULL)
    return -1;

  const unsigned n = l_allocn[resrq];
  l_entries[resrq].obtained = calloc(n, sizeof *l_entries[resrq].obtained);
  if (l_entries[resrq].obtained == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  l_entries[resrq].n = n;
  l_entries[resrq].i = 0;

  /* TODO: Consider using spec. constants to set the number of elements. */
  switch (resrq) {
    case YF_RESRQ_MDL:
      return init_mdl(l_entries+resrq, 1);
    case YF_RESRQ_MDL4:
      return init_mdl(l_entries+resrq, 4);
    case YF_RESRQ_MDL16:
      return init_mdl(l_entries+resrq, 16);
    case YF_RESRQ_MDL64:
      return init_mdl(l_entries+resrq, 64);
    case YF_RESRQ_TERR:
      return init_terr(l_entries+resrq);
    case YF_RESRQ_PART:
      return init_part(l_entries+resrq);
    default:
      assert(0);
      return -1;
  }
}

static void deinit_entry(int resrq) {
  assert(resrq >= 0 && resrq < YF_RESRQ_N);
  assert(l_entries[resrq].gst != NULL);
  assert(yf_getctx() != NULL);

  const int stages[] = {
    YF_STAGE_VERT,
    YF_STAGE_TESC,
    YF_STAGE_TESE,
    YF_STAGE_GEOM,
    YF_STAGE_FRAG,
    YF_STAGE_COMP
  };
  for (size_t i = 0; i < (sizeof stages / sizeof stages[0]); ++i) {
    const YF_stage *stg = yf_gstate_getstg(l_entries[resrq].gst, stages[i]);
    if (stg != NULL)
      yf_unldmod(yf_getctx(), stg->mod);
  }
  yf_dtable_deinit(yf_gstate_getdtb(l_entries[resrq].gst, YF_RESIDX_INST));
  yf_gstate_deinit(l_entries[resrq].gst);
  free(l_entries[resrq].obtained);
  memset(l_entries+resrq, 0, sizeof(L_entry));
}

static int init_mdl(L_entry *entry, unsigned elements) {
  YF_context ctx = yf_getctx();
  YF_pass pass = yf_getpass();

  assert(ctx != NULL && pass != NULL);

  /* stages */
  char *vert_path = make_shdpath(YF_NODEOBJ_MODEL, YF_STAGE_VERT, elements);
  char *frag_path = make_shdpath(YF_NODEOBJ_MODEL, YF_STAGE_FRAG, elements);
  if (vert_path == NULL || frag_path == NULL) {
    free(vert_path);
    free(frag_path);
    return -1;
  }

  YF_modid vert_mod;
  if (yf_loadmod(ctx, vert_path, &vert_mod) != 0) {
    free(vert_path);
    free(frag_path);
    return -1;
  }
  free(vert_path);

  YF_modid frag_mod;
  if (yf_loadmod(ctx, frag_path, &frag_mod) != 0) {
    yf_unldmod(ctx, vert_mod);
    free(frag_path);
    return -1;
  }
  free(frag_path);

  const YF_stage stgs[] = {
    {YF_STAGE_VERT, vert_mod, "main"},
    {YF_STAGE_FRAG, frag_mod, "main"}
  };
  const unsigned stg_n = sizeof stgs / sizeof stgs[0];

  /* dtables */
  const YF_dentry inst_ents[] = {
    {YF_RESBIND_INST, YF_DTYPE_UNIFORM, 1, NULL},
    {YF_RESBIND_TEX, YF_DTYPE_ISAMPLER, 1, NULL}
  };

  YF_dtable inst_dtb = yf_dtable_init(ctx, inst_ents,
      sizeof inst_ents / sizeof inst_ents[0]);

  if (inst_dtb == NULL || yf_dtable_alloc(inst_dtb, entry->n) != 0) {
    yf_unldmod(ctx, vert_mod);
    yf_unldmod(ctx, frag_mod);
    yf_dtable_deinit(inst_dtb);
    return -1;
  }

  const YF_dtable dtbs[] = {l_glob, inst_dtb};
  const unsigned dtb_n = sizeof dtbs / sizeof dtbs[0];

  /* vinputs */
  const YF_vattr attrs[] = {
    {YF_RESLOC_POS, YF_TYPEFMT_FLOAT3, 0},
    {YF_RESLOC_TC, YF_TYPEFMT_FLOAT2, offsetof(YF_vmdl, tc)},
    {YF_RESLOC_NORM, YF_TYPEFMT_FLOAT3, offsetof(YF_vmdl, norm)}
  };
  const YF_vinput vins[] = {
    {attrs, sizeof attrs / sizeof attrs[0], sizeof(YF_vmdl), YF_VRATE_VERT}
  };
  const unsigned vin_n = sizeof vins / sizeof vins[0];

  /* gstate */
  const YF_gconf conf = {
    pass,
    stgs,
    stg_n,
    dtbs,
    dtb_n,
    vins,
    vin_n,
    YF_PRIMITIVE_TRIANGLE,
    YF_POLYMODE_FILL,
    YF_CULLMODE_BACK,
    YF_WINDING_CCW
  };

  entry->gst = yf_gstate_init(ctx, &conf);
  if (entry->gst == NULL) {
    yf_unldmod(ctx, vert_mod);
    yf_unldmod(ctx, frag_mod);
    yf_dtable_deinit(inst_dtb);
    return -1;
  }

  return 0;
}

static int init_terr(L_entry *entry) {
  YF_context ctx = yf_getctx();
  YF_pass pass = yf_getpass();

  assert(ctx != NULL && pass != NULL);

  /* stages */
  char *vert_path = make_shdpath(YF_NODEOBJ_TERRAIN, YF_STAGE_VERT, 1);
  char *frag_path = make_shdpath(YF_NODEOBJ_TERRAIN, YF_STAGE_FRAG, 1);
  if (vert_path == NULL || frag_path == NULL) {
    free(vert_path);
    free(frag_path);
    return -1;
  }

  YF_modid vert_mod;
  if (yf_loadmod(ctx, vert_path, &vert_mod) != 0) {
    free(vert_path);
    free(frag_path);
    return -1;
  }
  free(vert_path);

  YF_modid frag_mod;
  if (yf_loadmod(ctx, frag_path, &frag_mod) != 0) {
    yf_unldmod(ctx, vert_mod);
    free(frag_path);
    return -1;
  }
  free(frag_path);

  const YF_stage stgs[] = {
    {YF_STAGE_VERT, vert_mod, "main"},
    {YF_STAGE_FRAG, frag_mod, "main"}
  };
  const unsigned stg_n = sizeof stgs / sizeof stgs[0];

  /* dtables */
  const YF_dentry inst_ents[] = {
    {YF_RESBIND_INST, YF_DTYPE_UNIFORM, 1, NULL},
    {YF_RESBIND_TEX, YF_DTYPE_ISAMPLER, 1, NULL},
    {YF_RESBIND_HMAP, YF_DTYPE_ISAMPLER, 1, NULL}
  };

  YF_dtable inst_dtb = yf_dtable_init(ctx, inst_ents,
      sizeof inst_ents / sizeof inst_ents[0]);

  if (inst_dtb == NULL || yf_dtable_alloc(inst_dtb, entry->n) != 0) {
    yf_unldmod(ctx, vert_mod);
    yf_unldmod(ctx, frag_mod);
    yf_dtable_deinit(inst_dtb);
    return -1;
  }

  const YF_dtable dtbs[] = {l_glob, inst_dtb};
  const unsigned dtb_n = sizeof dtbs / sizeof dtbs[0];

  /* vinputs */
  const YF_vattr attrs[] = {
    {YF_RESLOC_POS, YF_TYPEFMT_FLOAT3, 0},
    {YF_RESLOC_TC, YF_TYPEFMT_FLOAT2, offsetof(YF_vterr, tc)},
    {YF_RESLOC_NORM, YF_TYPEFMT_FLOAT3, offsetof(YF_vterr, norm)}
  };
  const YF_vinput vins[] = {
    {attrs, sizeof attrs / sizeof attrs[0], sizeof(YF_vterr), YF_VRATE_VERT}
  };
  const unsigned vin_n = sizeof vins / sizeof vins[0];

  /* gstate */
  const YF_gconf conf = {
    pass,
    stgs,
    stg_n,
    dtbs,
    dtb_n,
    vins,
    vin_n,
    YF_PRIMITIVE_TRIANGLE,
    YF_POLYMODE_FILL,
    YF_CULLMODE_BACK,
    YF_WINDING_CCW
  };

  entry->gst = yf_gstate_init(ctx, &conf);
  if (entry->gst == NULL) {
    yf_unldmod(ctx, vert_mod);
    yf_unldmod(ctx, frag_mod);
    yf_dtable_deinit(inst_dtb);
    return -1;
  }

  return 0;
}

static int init_part(L_entry *entry) {
  YF_context ctx = yf_getctx();
  YF_pass pass = yf_getpass();

  assert(ctx != NULL && pass != NULL);

  /* stages */
  char *vert_path = make_shdpath(YF_NODEOBJ_PARTICLE, YF_STAGE_VERT, 1);
  char *frag_path = make_shdpath(YF_NODEOBJ_PARTICLE, YF_STAGE_FRAG, 1);
  if (vert_path == NULL || frag_path == NULL) {
    free(vert_path);
    free(frag_path);
    return -1;
  }

  YF_modid vert_mod;
  if (yf_loadmod(ctx, vert_path, &vert_mod) != 0) {
    free(vert_path);
    free(frag_path);
    return -1;
  }
  free(vert_path);

  YF_modid frag_mod;
  if (yf_loadmod(ctx, frag_path, &frag_mod) != 0) {
    yf_unldmod(ctx, vert_mod);
    free(frag_path);
    return -1;
  }
  free(frag_path);

  const YF_stage stgs[] = {
    {YF_STAGE_VERT, vert_mod, "main"},
    {YF_STAGE_FRAG, frag_mod, "main"}
  };
  const unsigned stg_n = sizeof stgs / sizeof stgs[0];

  /* dtables */
  const YF_dentry inst_ents[] = {
    {YF_RESBIND_INST, YF_DTYPE_UNIFORM, 1, NULL},
    {YF_RESBIND_TEX, YF_DTYPE_ISAMPLER, 1, NULL}
  };

  YF_dtable inst_dtb = yf_dtable_init(ctx, inst_ents,
      sizeof inst_ents / sizeof inst_ents[0]);

  if (inst_dtb == NULL || yf_dtable_alloc(inst_dtb, entry->n) != 0) {
    yf_unldmod(ctx, vert_mod);
    yf_unldmod(ctx, frag_mod);
    yf_dtable_deinit(inst_dtb);
    return -1;
  }

  const YF_dtable dtbs[] = {l_glob, inst_dtb};
  const unsigned dtb_n = sizeof dtbs / sizeof dtbs[0];

  /* vinputs */
  const YF_vattr attrs[] = {
    {YF_RESLOC_POS, YF_TYPEFMT_FLOAT3, 0},
    {YF_RESLOC_CLR, YF_TYPEFMT_FLOAT4, offsetof(YF_vpart, clr)}
  };
  const YF_vinput vins[] = {
    {attrs, sizeof attrs / sizeof attrs[0], sizeof(YF_vpart), YF_VRATE_VERT}
  };
  const unsigned vin_n = sizeof vins / sizeof vins[0];

  /* gstate */
  const YF_gconf conf = {
    pass,
    stgs,
    stg_n,
    dtbs,
    dtb_n,
    vins,
    vin_n,
    YF_PRIMITIVE_POINT,
    YF_POLYMODE_FILL,
    YF_CULLMODE_BACK,
    YF_WINDING_CCW
  };

  entry->gst = yf_gstate_init(ctx, &conf);
  if (entry->gst == NULL) {
    yf_unldmod(ctx, vert_mod);
    yf_unldmod(ctx, frag_mod);
    yf_dtable_deinit(inst_dtb);
    return -1;
  }

  return 0;
}

static char *make_shdpath(int nodeobj, int stage, unsigned elements) {
  char *obj_name = NULL;
  switch (nodeobj) {
    case YF_NODEOBJ_MODEL:
      obj_name = "mdl";
      break;
    case YF_NODEOBJ_TERRAIN:
      obj_name = "terr";
      break;
    case YF_NODEOBJ_PARTICLE:
      obj_name = "part";
      break;
    case YF_NODEOBJ_QUAD:
      obj_name = "quad";
      break;
    case YF_NODEOBJ_LABEL:
      obj_name = "labl";
      break;
    case YF_NODEOBJ_LIGHT:
      obj_name = "lig";
      break;
    case YF_NODEOBJ_EFFECT:
      obj_name = "fx";
      break;
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      return NULL;
  }

  char *stg_name = NULL;
  switch (stage) {
    case YF_STAGE_VERT:
      stg_name = "vert";
      break;
    case YF_STAGE_TESC:
      stg_name = "tesc";
      break;
    case YF_STAGE_TESE:
      stg_name = "tese";
      break;
    case YF_STAGE_GEOM:
      stg_name = "geom";
      break;
    case YF_STAGE_FRAG:
      stg_name = "frag";
      break;
    case YF_STAGE_COMP:
      stg_name = "comp";
      break;
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      return NULL;
  }

  char elem_str[8] = {'\0'};
  if (elements > 1) {
    int sz = sizeof elem_str / sizeof elem_str[0];
    int r = snprintf(elem_str, sz, "%u", elements);
    if (r >= sz || r < 0) {
      yf_seterr(YF_ERR_OTHER, __func__);
      return NULL;
    }
  }

  const char *fmt = YF_SHD_DIR "%s%s.%s" YF_SHD_FILEEXT;
  char *str = NULL;
  int len = snprintf(str, 0, fmt, obj_name, elem_str, stg_name);
  if (len < 0) {
    yf_seterr(YF_ERR_OTHER, __func__);
    return NULL;
  }
  if ((str = malloc(++len)) == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  if (snprintf(str, len, fmt, obj_name, elem_str, stg_name) < 0) {
    yf_seterr(YF_ERR_OTHER, __func__);
    free(str);
    return NULL;
  }
  return str;
}
