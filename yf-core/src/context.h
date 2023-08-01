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

/* Data that is kept in a context but managed externally. */
typedef struct yf_ctxmgd {
    void *priv;
    void (*deinit_callb)(yf_context_t *);
} yf_ctxmgd_t;

struct yf_context {
    VkInstance instance;
    VkPhysicalDevice phy_dev;
    VkDevice device;

#define YF_QUEUE_GRAPH 0x1
#define YF_QUEUE_COMP  0x2
    unsigned queue_mask;
    VkQueue queue;
    int queue_i;

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

    yf_ctxmgd_t cmdp;
    yf_ctxmgd_t cmde;
    yf_ctxmgd_t lim;
    yf_ctxmgd_t stg;
    yf_ctxmgd_t splr;
};

#endif /* YF_CONTEXT_H */
