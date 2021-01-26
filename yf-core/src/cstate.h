/*
 * YF
 * cstate.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_CSTATE_H
#define YF_CSTATE_H

#include "yf-cstate.h"
#include "vk.h"

typedef struct YF_cstate_o {
  YF_context ctx;
  YF_stage stg;
  YF_dtable *dtbs;
  unsigned dtb_n;

  VkPipelineLayout layout;
  VkPipeline pipeline;
} YF_cstate_o;

#endif /* YF_CSTATE_H */
