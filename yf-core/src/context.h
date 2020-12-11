/*
 * YF
 * context.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_CONTEXT_H
#define YF_CONTEXT_H

#include "yf-context.h"
#include "vk.h"

/* Type defining data that is kept in a context but managed externally. */
typedef struct {
  void *priv;
  void (*deinit_callb)(YF_context);
} YF_ctxmgd;

typedef struct YF_context_o {
  VkInstance instance;
  VkPhysicalDevice phy_dev;
  VkDevice device;

  VkQueue graph_queue;
  int graph_queue_i;
  VkQueue comp_queue;
  int comp_queue_i;
  VkQueue pres_queue;
  int pres_queue_i;

  unsigned inst_version;
  VkPhysicalDeviceProperties dev_prop;
  VkPhysicalDeviceMemoryProperties mem_prop;

  VkPipelineCache pl_cache;

  char **layers;
  unsigned layer_n;
  char **inst_exts;
  unsigned inst_ext_n;
  char **dev_exts;
  unsigned dev_ext_n;

  YF_ctxmgd cmdp;
  YF_ctxmgd cmde;
  YF_ctxmgd lim;
  YF_ctxmgd stg;
} YF_context_o;

/* Debug macro. */
#ifdef YF_DEBUG
# define YF_CTX_PRINT(ctx) do { \
   printf("\n-- Context (debug) --"); \
   printf("\ng/c queue i: %d/%d", ctx->graph_queue_i, ctx->comp_queue_i); \
   printf("\ninst version: %u.%u", \
    VK_VERSION_MAJOR(ctx->inst_version), VK_VERSION_MINOR(ctx->inst_version)); \
   printf("\nlayer n: %u", ctx->layer_n); \
   for (unsigned i = 0; i < ctx->layer_n; ++i) \
    printf("\n\t%s", ctx->layers[i]); \
   printf("\ninst ext n: %u", ctx->inst_ext_n); \
   for (unsigned i = 0; i < ctx->inst_ext_n; ++i) \
    printf("\n\t%s", ctx->inst_exts[i]); \
   printf("\ndev ext n: %u", ctx->dev_ext_n); \
   for (unsigned i = 0; i < ctx->dev_ext_n; ++i) \
    printf("\n\t%s", ctx->dev_exts[i]); \
   printf("\n--\n"); } while (0)
#endif

#endif /* YF_CONTEXT_H */
