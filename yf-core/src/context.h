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
  VkPhysicalDeviceFeatures features;

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

#endif /* YF_CONTEXT_H */
