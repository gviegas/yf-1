/*
 * YF
 * cstate.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_CSTATE_H
#define YF_CSTATE_H

#include "yf-cstate.h"
#include "vk.h"

struct yf_cstate {
    yf_context_t *ctx;
    yf_stage_t stg;
    yf_dtable_t **dtbs;
    unsigned dtb_n;

    VkPipelineLayout layout;
    VkPipeline pipeline;
};

#endif /* YF_CSTATE_H */
