/*
 * YF
 * stage.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "stage.h"
#include "context.h"
#include "hashset.h"
#include "error.h"

#define YF_MODWRD 4
#define YF_MODBLK (YF_MODWRD * 256)

/* Type defining stage variables stored in a context. */
typedef struct {
  YF_hashset mods;
  YF_modid modid;
} L_priv;

/* Type defining a key/value for the module's hashset. */
typedef struct {
  YF_modid key;
  VkShaderModule value;
} L_kv;

/* Destroys the 'L_priv' data stored in a given context. */
static void destroy_priv(YF_context ctx);

/* Hashes a 'L_kv'. */
static size_t hash_kv(const void *x);

/* Compares a 'L_kv' to another. */
static int cmp_kv(const void *a, const void *b);

int yf_loadmod(YF_context ctx, const char *pathname, YF_modid *mod) {
  assert(ctx != NULL);
  assert(pathname != NULL);

  L_priv *priv = ctx->stg.priv;
  if (priv == NULL) {
    if ((ctx->stg.priv = malloc(sizeof *priv)) == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    priv = ctx->stg.priv;
    if ((priv->mods = yf_hashset_init(hash_kv, cmp_kv)) == NULL) {
      free(ctx->stg.priv);
      ctx->stg.priv = NULL;
      return -1;
    }
    priv->modid = 0;
    ctx->stg.deinit_callb = destroy_priv;
  }

  FILE *f = fopen(pathname, "r");
  if (f == NULL) {
    yf_seterr(YF_ERR_NOFILE, __func__);
    return -1;
  }

  unsigned char *buf = NULL;
  size_t n = 0;
  do {
    void *tmp = realloc(buf, n + YF_MODBLK);
    if (tmp == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      free(buf);
      fclose(f);
      return -1;
    }
    buf = tmp;
    n += fread(buf+n, 1, YF_MODBLK, f);
  } while (!feof(f));

  fclose(f);

  if (n == 0 || n % YF_MODWRD != 0) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    free(buf);
    return -1;
  }

  L_kv *kv = malloc(sizeof(L_kv));
  if (kv == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(buf);
    return -1;
  }
  kv->key = ++priv->modid;

  VkShaderModuleCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .codeSize = n,
    .pCode = (const uint32_t *)buf
  };
  VkResult res = vkCreateShaderModule(ctx->device, &info, NULL, &kv->value);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    free(buf);
    free(kv);
    return -1;
  }

  free(buf);
  yf_hashset_insert(priv->mods, kv);
  *mod = kv->key;
  return 0;
}

void yf_unldmod(YF_context ctx, YF_modid mod) {
  assert(ctx != NULL);

  if (ctx->stg.priv == NULL)
    return;

  L_priv *priv = ctx->stg.priv;
  L_kv key = {mod, VK_NULL_HANDLE};
  L_kv *kv = yf_hashset_search(priv->mods, &key);
  if (kv != NULL) {
    yf_hashset_remove(priv->mods, kv);
    vkDestroyShaderModule(ctx->device, kv->value, NULL);
    free(kv);
  }
}

VkShaderModule yf_getmod(YF_context ctx, YF_modid mod) {
  assert(ctx != NULL);

  if (ctx->stg.priv == NULL)
    return VK_NULL_HANDLE;

  L_priv *priv = ctx->stg.priv;
  L_kv key = {mod, VK_NULL_HANDLE};
  L_kv *kv = yf_hashset_search(priv->mods, &key);
  if (kv != NULL)
    return kv->value;
  return VK_NULL_HANDLE;
}

static void destroy_priv(YF_context ctx) {
  assert(ctx != NULL);

  if (ctx->stg.priv == NULL)
    return;

  L_priv *priv = ctx->stg.priv;
  YF_iter it = YF_NILIT;
  do
    free(yf_hashset_next(priv->mods, &it));
  while (!YF_IT_ISNIL(it));
  yf_hashset_deinit(priv->mods);
  free(priv);
  ctx->stg.priv = NULL;
}

static size_t hash_kv(const void *x) {
  return ((L_kv *)x)->key ^ 0xe353d215;
}

static int cmp_kv(const void *a, const void *b) {
  return ((L_kv *)a)->key != ((L_kv *)b)->key;
}
