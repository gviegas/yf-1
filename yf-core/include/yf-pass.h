/*
 * YF
 * yf-pass.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_PASS_H
#define YF_YF_PASS_H

#include "yf/com/yf-defs.h"

#include "yf-context.h"
#include "yf-image.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a render pass configuration.
 */
typedef struct YF_pass_o *YF_pass;

/**
 * Opaque type defining a render target for a pass.
 */
typedef struct YF_target_o *YF_target;

/**
 * Load operations.
 */
#define YF_LOADOP_UNDEF 0
#define YF_LOADOP_LOAD  1

/**
 * Store operations.
 */
#define YF_STOREOP_UNDEF 0
#define YF_STOREOP_STORE 1

/**
 * Type describing a color attachment for use in a pass.
 */
typedef struct {
    int pixfmt;
    int samples;
    int loadop;
    int storeop;
} YF_colordsc;

/**
 * Type describing a depth/stencil attachment for use in a pass.
 */
typedef struct {
    int pixfmt;
    int samples;
    int depth_loadop;
    int depth_storeop;
    int stencil_loadop;
    int stencil_storeop;
} YF_depthdsc;

/**
 * Type defining the resource of a target's attachment.
 */
typedef struct {
    YF_image img;
    unsigned layer_base;
} YF_attach;

/**
 * Initializes a new pass.
 *
 * @param ctx: The context.
 * @param colors: The descriptions for color attachments.
 * @param color_n: The number of color descriptions.
 * @param resolves: The descriptions for resolve attachments. If not 'NULL',
 *  it must contain 'color_n' entries.
 * @param depth_stencil: The description for depth/stencil attachment.
 *  Can be 'NULL'.
 * @return: On success, returns a new pass. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_pass yf_pass_init(YF_context ctx, const YF_colordsc *colors,
                     unsigned color_n, const YF_colordsc *resolves,
                     const YF_depthdsc *depth_stencil);

/**
 * Makes a new target for use with a given pass.
 *
 * @param pass: The pass that this target will be compatible with.
 * @param dim: The size of the framebuffer.
 * @param layers: The number of layers in the target.
 * @param colors: The color attachments.
 * @param color_n: The number of color attachments.
 * @param resolves: The resolve attachments.
 * @param depth_stencil: The depth/stencil attachment.
 * @return: On success, returns a new target. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_target yf_pass_maketarget(YF_pass pass, YF_dim2 dim, unsigned layers,
                             const YF_attach *colors, unsigned color_n,
                             const YF_attach *resolves,
                             const YF_attach *depth_stencil);

/**
 * Unmakes a pass' target.
 *
 * @param pass: The pass that produced the target to be unmade.
 * @param tgt: The target to unmake.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_pass_unmktarget(YF_pass pass, YF_target tgt);

/**
 * Deinitializes a pass.
 *
 * @param pass: The pass to deinitialize. Can be 'NULL'.
 */
void yf_pass_deinit(YF_pass pass);

YF_DECLS_END

#endif /* YF_YF_PASS_H */
