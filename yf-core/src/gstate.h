/*
 * YF
 * gstate.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_GSTATE_H
#define YF_GSTATE_H

#include "yf-gstate.h"
#include "vk.h"

typedef struct YF_gstate_o {
  YF_context ctx;
  YF_pass pass;
  YF_dtable *dtbs;
  unsigned dtb_n;

  VkPipelineLayout layout;
  VkPipeline pipeline;
} YF_gstate_o;

/* Converts from a 'YF_PRIMITIVE' value. */
#define YF_PRIMITIVE_FROM(prim, to) do { \
  if (prim == YF_PRIMITIVE_POINT) \
    to = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; \
  else if (prim == YF_PRIMITIVE_LINE) \
    to = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; \
  else if (prim == YF_PRIMITIVE_TRIANGLE) \
    to = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; \
  else \
    to = INT_MAX; \
  } while (0)

/* Converts from a 'YF_POLYMODE' value. */
#define YF_POLYMODE_FROM(pm, to) do { \
  if (pm == YF_POLYMODE_FILL) to = VK_POLYGON_MODE_FILL; \
  else if (pm == YF_POLYMODE_LINE) to = VK_POLYGON_MODE_LINE; \
  else if (pm == YF_POLYMODE_POINT) to = VK_POLYGON_MODE_POINT; \
  else to = INT_MAX; } while (0)

/* Converts from a 'YF_CULLMODE' value. */
#define YF_CULLMODE_FROM(cm, to) do { \
  if (cm == YF_CULLMODE_NONE) to = VK_CULL_MODE_NONE; \
  else if (cm == YF_CULLMODE_FRONT) to = VK_CULL_MODE_FRONT_BIT; \
  else if (cm == YF_CULLMODE_BACK) to = VK_CULL_MODE_BACK_BIT; \
  else if (cm == YF_CULLMODE_ANY) to = VK_CULL_MODE_FRONT_AND_BACK; \
  else to = INT_MAX; } while (0)

/* Converts from a 'YF_FRONTFACE' value. */
#define YF_FRONTFACE_FROM(ff, to) do { \
  if (ff == YF_FRONTFACE_CW) to = VK_FRONT_FACE_CLOCKWISE; \
  else if (ff == YF_FRONTFACE_CCW) to = VK_FRONT_FACE_COUNTER_CLOCKWISE; \
  else to = INT_MAX; } while (0)

#endif /* YF_GSTATE_H */
