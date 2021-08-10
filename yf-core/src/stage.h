/*
 * YF
 * stage.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_STAGE_H
#define YF_STAGE_H

#include "yf-stage.h"
#include "vk.h"

/* Checks whether or not a value represents a single stage. */
#define YF_STAGE_ONE(stage) ( (stage) == YF_STAGE_VERT || \
                              (stage) == YF_STAGE_TESC || \
                              (stage) == YF_STAGE_TESE || \
                              (stage) == YF_STAGE_GEOM || \
                              (stage) == YF_STAGE_FRAG || \
                              (stage) == YF_STAGE_COMP )

/* Checks whether or not a combination of stages is invalid for graphics. */
#define YF_STAGE_INVGRAPH(mask) ( ((mask) & YF_STAGE_VERT) == 0 || \
                                  ((mask) & YF_STAGE_COMP) != 0 )

/* Checks whether or not a combination of stages is invalid for compute. */
#define YF_STAGE_INVCOMP(mask) ((mask) != YF_STAGE_COMP)

/* Converts from a stage mask. */
#define YF_STAGE_FROM(st, to) do { \
    to = 0; \
    if (((st) & YF_STAGE_VERT) != 0) \
        to |= VK_SHADER_STAGE_VERTEX_BIT; \
    if (((st) & YF_STAGE_TESC) != 0) \
        to |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT; \
    if (((st) & YF_STAGE_TESE) != 0) \
        to |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; \
    if (((st) & YF_STAGE_GEOM) != 0) \
        to |= VK_SHADER_STAGE_GEOMETRY_BIT; \
    if (((st) & YF_STAGE_FRAG) != 0) \
        to |= VK_SHADER_STAGE_FRAGMENT_BIT; \
    if (((st) & YF_STAGE_COMP) != 0) \
        to |= VK_SHADER_STAGE_COMPUTE_BIT; } while (0)

/* Gets the underlying shader module for a given 'YF_shdid'. */
VkShaderModule yf_getshd(YF_context ctx, YF_shdid shd);

#endif /* YF_STAGE_H */
