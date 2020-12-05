/*
 * YF
 * resmgr.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "resmgr.h"
#include "coreobj.h"
#include "node.h"
#include "vertex.h"
#include "error.h"

#ifdef YF_DEBUG
# define YF_RESMGR_PRINT(info) do { \
   printf("\n-- Resmgr (debug) --"); \
   printf("\n> %s\n", info); \
   for (unsigned i = 0; i < YF_RESRQ_N; ++i) { \
    printf("\nRQ#%u: gst=%p n=%u i=%u\n  obtd[ ", \
     i, (void *)l_entries[i].gst, l_entries[i].n, l_entries[i].i); \
    for (unsigned j = 0; j < l_entries[i].n; ++j) \
     printf("%d ", l_entries[i].obtained[j]); \
    printf("]\n"); \
   } \
   printf("\n--\n"); } while (0)
#endif

#ifndef YF_SHD_DIR
# define YF_SHD_DIR "bin/"
#endif
#ifndef YF_SHD_FILEEXT
# define YF_SHD_FILEEXT ".spv"
#endif

#define YF_ALLOCN_MDL   64
#define YF_ALLOCN_MDL4  16
#define YF_ALLOCN_MDL16 16
#define YF_ALLOCN_MDL64  4

/* Type defining an entry in the resource list. */
typedef struct {
  YF_gstate gst;
  char *obtained;
  unsigned n;
  unsigned i;
} L_entry;

/* List of resources, indexed by 'resrq' values. */
static L_entry l_entries[YF_RESRQ_N] = {0};

/* Sizes used for instance allocations, indexed by 'resrq' values. */
static unsigned l_allocn[YF_RESRQ_N] = {
  [YF_RESRQ_MDL]   = YF_ALLOCN_MDL,
  [YF_RESRQ_MDL4]  = YF_ALLOCN_MDL4,
  [YF_RESRQ_MDL16] = YF_ALLOCN_MDL16,
  [YF_RESRQ_MDL64] = YF_ALLOCN_MDL64
};

/* Initializes the entry of a given 'resrq' value. */
static int init_entry(int resrq);

/* Deinitializes the entry of a given 'resrq' value. */
static void deinit_entry(int resrq);

/* Initializes the entry of a model resource. */
static int init_mdl(L_entry *entry, unsigned elements);

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
#ifdef YF_DEBUG
  YF_RESMGR_PRINT(__func__);
#endif
  return gst;
}

void yf_resmgr_yield(int resrq, unsigned inst_alloc) {
  assert(resrq >= 0 && resrq < YF_RESRQ_N);
  assert(inst_alloc < l_entries[resrq].n);

  l_entries[resrq].obtained[inst_alloc] = 0;
  l_entries[resrq].i = inst_alloc;
#ifdef YF_DEBUG
  YF_RESMGR_PRINT(__func__);
#endif
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
    /* this does the right thing when n is zero */
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

  if (l_allocn[resrq] == 0 || l_entries[resrq].gst != NULL) {
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
}

static int init_entry(int resrq) {
  assert(resrq >= 0 && resrq < YF_RESRQ_N);

  const unsigned n = l_allocn[resrq];
  assert(n > 0);
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
    default:
      assert(0);
      return -1;
  }
}

static void deinit_entry(int resrq) {
  assert(resrq >= 0 && resrq < YF_RESRQ_N);
  assert(l_entries[resrq].gst != NULL);

  /* TODO: May want to unload shader modules here. */
  yf_dtable_deinit(yf_gstate_getdtb(l_entries[resrq].gst, YF_RESIDX_GLOB));
  yf_dtable_deinit(yf_gstate_getdtb(l_entries[resrq].gst, YF_RESIDX_INST));
  yf_gstate_deinit(l_entries[resrq].gst);
  free(l_entries[resrq].obtained);
  memset(l_entries+resrq, 0, sizeof(L_entry));
}

static int init_mdl(L_entry *entry, unsigned elements) {
  YF_context ctx = yf_getctx();
  YF_pass pass = yf_getpass();
  if (ctx == NULL || pass == NULL)
    return -1;

  /* stages */
  char *vert_path = make_shdpath(YF_NODEOBJ_MODEL, YF_STAGE_VERT, elements);
  char *frag_path = make_shdpath(YF_NODEOBJ_MODEL, YF_STAGE_FRAG, elements);
  if (vert_path == NULL || frag_path == NULL) {
    free(vert_path);
    free(frag_path);
    return -1;
  }
  YF_modid vert_mod, frag_mod;
  if (yf_loadmod(ctx, vert_path, &vert_mod) != 0) {
    free(vert_path);
    free(frag_path);
    return -1;
  }
  if (yf_loadmod(ctx, frag_path, &frag_mod) != 0) {
    yf_unldmod(ctx, vert_mod);
    free(vert_path);
    free(frag_path);
    return -1;
  }
  free(vert_path);
  free(frag_path);

  const YF_stage stgs[] = {
    {YF_STAGE_VERT, vert_mod, "main"},
    {YF_STAGE_FRAG, frag_mod, "main"}
  };
  const unsigned stg_n = sizeof stgs / sizeof stgs[0];

  /* dtables */
  const YF_dentry glob_ents[] = {
    {YF_RESBIND_UGLOB, YF_DTYPE_UNIFORM, 1, NULL}
  };
  const YF_dentry inst_ents[] = {
    {YF_RESBIND_UINST, YF_DTYPE_UNIFORM, 1, NULL},
    {YF_RESBIND_ISTEX, YF_DTYPE_ISAMPLER, 1, NULL}
  };
  unsigned ent_sz;

  ent_sz = sizeof glob_ents / sizeof glob_ents[0];
  YF_dtable glob_dtb = yf_dtable_init(ctx, glob_ents, ent_sz);
  ent_sz = sizeof inst_ents / sizeof inst_ents[0];
  YF_dtable inst_dtb = yf_dtable_init(ctx, inst_ents, ent_sz);

  if (glob_dtb == NULL ||
    inst_dtb == NULL ||
    yf_dtable_alloc(glob_dtb, 1) != 0 ||
    yf_dtable_alloc(inst_dtb, entry->n) != 0)
  {
    yf_dtable_deinit(glob_dtb);
    yf_dtable_deinit(inst_dtb);
    yf_unldmod(ctx, vert_mod);
    yf_unldmod(ctx, frag_mod);
    return -1;
  }
  const YF_dtable dtbs[] = {glob_dtb, inst_dtb};
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
    YF_FRONTFACE_CCW
  };
  entry->gst = yf_gstate_init(ctx, &conf);
  if (entry->gst == NULL) {
    for (unsigned i = 0; i < stg_n; ++i)
      yf_unldmod(ctx, stgs[i].mod);
    for (unsigned i = 0; i < dtb_n; ++i)
      yf_dtable_deinit(dtbs[i]);
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
