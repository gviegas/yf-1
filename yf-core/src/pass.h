/*
 * YF
 * pass.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_PASS_H
#define YF_PASS_H

#include "yf-pass.h"
#include "image.h"
#include "vk.h"

typedef struct YF_pass_o {
    YF_context ctx;
    unsigned color_n;
    unsigned resolve_n;
    unsigned depth_n;
    YF_target *tgts;
    unsigned tgt_cap;
    unsigned tgt_n;
    unsigned tgt_i;
    VkRenderPass ren_pass;
} YF_pass_o;

typedef struct YF_target_o {
    YF_pass pass;
    YF_dim2 dim;
    unsigned layers;
    YF_iview *iviews;
    unsigned iview_n;
    YF_image *imgs;
    unsigned *lays_base;
    VkFramebuffer framebuf;
} YF_target_o;

/* Converts from a load op value. */
#define YF_LOADOP_FROM(op, to) do { \
    switch (op) { \
    case YF_LOADOP_UNDEF: \
        to = VK_ATTACHMENT_LOAD_OP_DONT_CARE; \
        break; \
    case YF_LOADOP_LOAD: \
        to = VK_ATTACHMENT_LOAD_OP_LOAD; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

/* Converts from a store op value. */
#define YF_STOREOP_FROM(op, to) do { \
    switch (op) { \
    case YF_STOREOP_UNDEF: \
        to = VK_ATTACHMENT_STORE_OP_DONT_CARE; \
        break; \
    case YF_STOREOP_STORE: \
        to = VK_ATTACHMENT_STORE_OP_STORE; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

#endif /* YF_PASS_H */
