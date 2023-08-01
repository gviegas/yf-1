/*
 * YF
 * yf-pass.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
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
typedef struct yf_pass yf_pass_t;

/**
 * Opaque type defining a render target for a pass.
 */
typedef struct yf_target yf_target_t;

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
typedef struct yf_colordsc {
    int pixfmt;
    int samples;
    int loadop;
    int storeop;
} yf_colordsc_t;

/**
 * Type describing a depth/stencil attachment for use in a pass.
 */
typedef struct yf_depthdsc {
    int pixfmt;
    int samples;
    int depth_loadop;
    int depth_storeop;
    int stencil_loadop;
    int stencil_storeop;
} yf_depthdsc_t;

/**
 * Type defining the resource of a target's attachment.
 */
typedef struct yf_attach {
    yf_image_t *img;
    unsigned layer_base;
} yf_attach_t;

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
yf_pass_t *yf_pass_init(yf_context_t *ctx, const yf_colordsc_t *colors,
                        unsigned color_n, const yf_colordsc_t *resolves,
                        const yf_depthdsc_t *depth_stencil);

/**
 * Makes a new target for use with a given pass.
 *
 * @param pass: The pass that this target will be compatible with.
 * @param dim: The size of the framebuffer.
 * @param layers: The number of layers in the target.
 * @param colors: The color attachments, as specified in 'pass'.
 * @param resolves: The resolve attachments, as specified in 'pass'.
 * @param depth_stencil: The depth/stencil attachment, as specified in 'pass'.
 * @return: On success, returns a new target. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_target_t *yf_pass_maketarget(yf_pass_t *pass, yf_dim2_t dim, unsigned layers,
                                const yf_attach_t *colors,
                                const yf_attach_t *resolves,
                                const yf_attach_t *depth_stencil);

/**
 * Unmakes a pass' target.
 *
 * @param pass: The pass that produced the target to be unmade.
 * @param tgt: The target to unmake.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_pass_unmktarget(yf_pass_t *pass, yf_target_t *tgt);

/**
 * Deinitializes a pass.
 *
 * @param pass: The pass to deinitialize. Can be 'NULL'.
 */
void yf_pass_deinit(yf_pass_t *pass);

YF_DECLS_END

#endif /* YF_YF_PASS_H */
