/*
 * YF
 * gstate.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_GSTATE_H
#define YF_GSTATE_H

#include "yf-gstate.h"
#include "vk.h"

typedef struct YF_gstate_o {
    YF_context ctx;
    YF_pass pass;
    YF_stage *stgs;
    unsigned stg_n;
    YF_dtable *dtbs;
    unsigned dtb_n;

    VkPipelineLayout layout;
    VkPipeline pipeline;
} YF_gstate_o;

/* Converts from a 'YF_TOPOLOGY' value. */
#define YF_TOPOLOGY_FROM(tl, to) do { \
    switch (tl) { \
    case YF_TOPOLOGY_POINT: \
        to = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; \
        break; \
    case YF_TOPOLOGY_LINE: \
        to = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; \
        break; \
    case YF_TOPOLOGY_TRIANGLE: \
        to = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; \
        break; \
    case YF_TOPOLOGY_LNSTRIP: \
        to = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; \
        break; \
    case YF_TOPOLOGY_TRISTRIP: \
        to = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; \
        break; \
    case YF_TOPOLOGY_TRIFAN: \
        to = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

/* Converts from a 'YF_POLYMODE' value. */
#define YF_POLYMODE_FROM(pm, to) do { \
    switch (pm) { \
    case YF_POLYMODE_FILL: \
        to = VK_POLYGON_MODE_FILL; \
        break; \
    case YF_POLYMODE_LINE: \
        to = VK_POLYGON_MODE_LINE; \
        break; \
    case YF_POLYMODE_POINT: \
        to = VK_POLYGON_MODE_POINT; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

/* Converts from a 'YF_CULLMODE' value. */
#define YF_CULLMODE_FROM(cm, to) do { \
    switch (cm) { \
    case YF_CULLMODE_NONE: \
        to = VK_CULL_MODE_NONE; \
        break; \
    case YF_CULLMODE_FRONT: \
        to = VK_CULL_MODE_FRONT_BIT; \
        break; \
    case YF_CULLMODE_BACK: \
        to = VK_CULL_MODE_BACK_BIT; \
        break; \
    case YF_CULLMODE_ANY: \
        to = VK_CULL_MODE_FRONT_AND_BACK; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

/* Converts from a 'YF_WINDING' value. */
#define YF_WINDING_FROM(wd, to) do { \
    switch (wd) { \
    case YF_WINDING_CW: \
        to = VK_FRONT_FACE_CLOCKWISE; \
        break; \
    case YF_WINDING_CCW: \
        to = VK_FRONT_FACE_COUNTER_CLOCKWISE; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

#endif /* YF_GSTATE_H */
