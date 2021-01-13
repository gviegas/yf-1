/*
 * YF
 * texture.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <yf/com/yf-hashset.h>
#include <yf/com/yf-error.h>
#include <yf/core/yf-image.h>

#include "texture.h"
#include "coreobj.h"
#include "data-bmp.h"

#ifdef YF_DEBUG
# include <stdio.h>
# define YF_TEX_PRINT(tex) do { \
   printf("\n-- Texture (debug) --"); \
   printf("\nimge: %p", (void *)tex->imge); \
   printf("\nlayer: %u", tex->layer); \
   printf("\nimge - img: %p", (void *)tex->imge->img); \
   /* \
   printf("\nimge - lay_used:\n"); \
   for (unsigned i = 0; i < lay_cap; ++i) \
    printf("%d'", tex->imge->lay_used[i]); \
   */ \
   printf("\nimge - lay_n: %u", tex->imge->lay_n); \
   printf("\nimge - lay_i: %u", tex->imge->lay_i); \
   printf("\n--\n"); } while (0)
#endif

#undef YF_LAYCAP
#define YF_LAYCAP 64

/* Type defining an image object and associated layer state. */
/* XXX: This assumes that a single image object will suffice. */
typedef struct {
  YF_image img;
  char *lay_used;
  unsigned lay_n;
  unsigned lay_i;
} L_imge;

/* Type holding key & value for use in the image hashset. */
/* TODO: Add levels and samples as key. */
typedef struct {
  struct { int pixfmt; YF_dim2 dim; } key;
  L_imge value;
} L_kv;

struct YF_texture_o {
  L_imge *imge;
  unsigned layer;
};

/* Global context. */
static YF_context l_ctx = NULL;

/* Hashset containing all created images. */
static YF_hashset l_imges = NULL;

/* Copies texture data to image and updates texture object. */
static int copy_data(YF_texture tex, const YF_texdt *data);

/* Hashes a 'L_kv'. */
static size_t hash_kv(const void *x);

/* Compares a 'L_kv' to another. */
static int cmp_kv(const void *a, const void *b);

YF_texture yf_texture_init(int filetype, const char *pathname) {
  YF_texdt data = {0};
  switch (filetype) {
    case YF_FILETYPE_INTERNAL:
      /* TODO */
      assert(0);
    case YF_FILETYPE_PNG:
      /* TODO */
      assert(0);
    case YF_FILETYPE_BMP:
      if (yf_loadbmp(pathname, &data) != 0)
        return NULL;
      break;
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      return NULL;
  }
  YF_texture tex = yf_texture_initdt(&data);
  free(data.data);
  return tex;
}

void yf_texture_deinit(YF_texture tex) {
  if (tex == NULL)
    return;

  YF_dim3 dim;
  int pixfmt;
  yf_image_getval(tex->imge->img, &pixfmt, &dim, NULL, NULL, NULL);

  const L_kv key = {{pixfmt, {dim.width, dim.height}}, {0}};
  L_kv *val = yf_hashset_search(l_imges, &key);
  assert(val != NULL);

  if (val->value.lay_n > 1) {
    val->value.lay_used[tex->layer] = 0;
    val->value.lay_n--;
  } else {
    yf_hashset_remove(l_imges, val);
    yf_image_deinit(val->value.img);
    free(val->value.lay_used);
    free(val);
  }
  free(tex);
}

YF_texture yf_texture_initdt(const YF_texdt *data) {
  assert(data != NULL);
#ifdef YF_DEBUG
  YF_TEXDT_PRINT(data);
#endif

  if (l_ctx == NULL && (l_ctx = yf_getctx()) == NULL)
    return NULL;
  if (l_imges == NULL && (l_imges = yf_hashset_init(hash_kv, cmp_kv)) == NULL)
    return NULL;

  YF_texture tex = calloc(1, sizeof(struct YF_texture_o));
  if (tex == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  if (copy_data(tex, data) != 0) {
    yf_texture_deinit(tex);
    tex = NULL;
  }
#ifdef YF_DEBUG
  if (tex != NULL)
    YF_TEX_PRINT(tex);
#endif
  return tex;
}

int yf_texture_setdata(YF_texture tex, YF_off2 off, YF_dim2 dim,
    const void *data)
{
  assert(tex != NULL);
  assert(data != NULL);

  const YF_off3 off3 = {off.x, off.y, 0};
  const YF_dim3 dim3 = {dim.width, dim.height, 1};
  /* TODO: Mip level. */
  return yf_image_copy(tex->imge->img, off3, dim3, tex->layer, 1, data);
}

int yf_texture_copyres(YF_texture tex, YF_dtable dtb, unsigned alloc_i,
    unsigned binding, unsigned element)
{
  assert(tex != NULL);
  assert(dtb != NULL);

  YF_slice elem = {element, 1};
  return yf_dtable_copyimg(dtb, alloc_i, binding, elem,
      &tex->imge->img, &tex->layer);
}

static int copy_data(YF_texture tex, const YF_texdt *data) {
  const L_kv key = {{data->pixfmt, data->dim}, {0}};
  L_kv *val = yf_hashset_search(l_imges, &key);

  if (val == NULL) {
    if ((val = malloc(sizeof *val)) == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    YF_dim3 dim = {data->dim.width, data->dim.height, 1};
    val->value.img = yf_image_init(l_ctx, data->pixfmt, dim, YF_LAYCAP, 1, 1);
    if (val->value.img == NULL) {
      free(val);
      return -1;
    }
    val->value.lay_used = calloc(YF_LAYCAP, sizeof *val->value.lay_used);
    if (val->value.lay_used == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      yf_image_deinit(val->value.img);
      free(val);
      return -1;
    }
    val->value.lay_n = 0;
    val->value.lay_i = 0;
    val->key.pixfmt = data->pixfmt;
    val->key.dim = data->dim;
    if (yf_hashset_insert(l_imges, val) != 0) {
      yf_image_deinit(val->value.img);
      free(val->value.lay_used);
      free(val);
      return -1;
    }
  }

  int pixfmt;
  YF_dim3 dim;
  unsigned layers, levels, samples;
  yf_image_getval(val->value.img, &pixfmt, &dim, &layers, &levels, &samples);

  if (val->value.lay_n == layers) {
    unsigned new_lay_cap = layers << 1;
    YF_image new_img = yf_image_init(l_ctx, pixfmt, dim, new_lay_cap, levels,
        samples);
    if (new_img == NULL)
      return -1;
    YF_cmdbuf cb = yf_cmdbuf_get(l_ctx, YF_CMDBUF_GRAPH);
    if (cb == NULL && (cb = yf_cmdbuf_get(l_ctx, YF_CMDBUF_COMP)) == NULL) {
      yf_image_deinit(new_img);
      return -1;
    }
    yf_cmdbuf_copyimg(cb, new_img, 0, val->value.img, 0, layers);
    /*
    if (yf_cmdbuf_end(cb) != 0 || yf_context_exec(l_ctx) != 0) {
      yf_image_deinit(new_img);
      return -1;
    }
    */
    yf_cmdbuf_sync(cb);
    if (yf_cmdbuf_end(cb) != 0) {
      yf_image_deinit(new_img);
      return -1;
    }
    char *new_lay_used = realloc(val->value.lay_used, new_lay_cap);
    if (new_lay_used == NULL) {
      yf_image_deinit(new_img);
      return -1;
    }
    memset(new_lay_used+layers, 0, layers);
    /* XXX: Will crash if old image is in use. */
    yf_image_deinit(val->value.img);
    val->value.img = new_img;
    val->value.lay_used = new_lay_used;
    val->value.lay_i = layers;
    layers = new_lay_cap;
  }

  unsigned layer = val->value.lay_i;
  unsigned lay_cap = layers;
  for (; val->value.lay_used[layer]; layer = (layer+1) % lay_cap);
  const YF_off3 off = {0, 0, 0};

  if (yf_image_copy(val->value.img, off, dim, layer, 0, data->data) != 0) {
    if (val->value.lay_n == 0) {
      yf_hashset_remove(l_imges, val);
      yf_image_deinit(val->value.img);
      free(val->value.lay_used);
      free(val);
    }
    return -1;
  }

  val->value.lay_used[layer] = 1;
  val->value.lay_n++;
  val->value.lay_i = (layer+1) % lay_cap;
  tex->imge = &val->value;
  tex->layer = layer;
  return 0;
}

static size_t hash_kv(const void *x) {
  const L_kv *kv = x;
  return kv->key.pixfmt ^ kv->key.dim.width ^ kv->key.dim.height ^ 3258114451;
}

static int cmp_kv(const void *a, const void *b) {
  const L_kv *kv1 = a;
  const L_kv *kv2 = b;
  return !(kv1->key.pixfmt == kv2->key.pixfmt &&
      kv1->key.dim.width == kv2->key.dim.width &&
      kv1->key.dim.height == kv2->key.dim.height);
}
