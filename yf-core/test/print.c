/*
 * YF
 * print.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "print.h"

#undef YF_PTITLE
#define YF_PTITLE printf("\n[YF] OUTPUT (%s):", __func__)

void yf_print_ctx(YF_context ctx)
{
    YF_PTITLE;

    printf("\nqueue indices:\n subm: %d\n pres: %d", ctx->queue_i,
           ctx->pres_queue_i);
    printf("\nqueue mask:\n graph? %s\n comp? %s",
           ctx->queue_mask & YF_QUEUE_GRAPH ? "yes" : "no",
           ctx->queue_mask & YF_QUEUE_COMP ? "yes" : "no");
    printf("\ninstance version: %u.%u", VK_VERSION_MAJOR(ctx->inst_version),
           VK_VERSION_MINOR(ctx->inst_version));

    printf("\nlayers: #%u", ctx->layer_n);
    for (unsigned i = 0; i < ctx->layer_n; ++i)
        printf("\n\t%s", ctx->layers[i]);
    printf("\ninstance extensions: #%u", ctx->inst_ext_n);
    for (unsigned i = 0; i < ctx->inst_ext_n; ++i)
        printf("\n\t%s", ctx->inst_exts[i]);
    printf("\ndevice extensions: #%u", ctx->dev_ext_n);
    for (unsigned i = 0; i < ctx->dev_ext_n; ++i)
        printf("\n\t%s", ctx->dev_exts[i]);

    printf("\n\n");
}

void yf_print_lim(const YF_limits *lim)
{
    YF_PTITLE;

    printf("\nmemory - max objects: %lu", lim->memory.obj_max);

    printf("\nbuffer - max size: %lu", lim->buffer.sz_max);

    printf("\nimage - max 1d: %u", lim->image.dim_1d_max);
    printf("\nimage - max 2d: %u", lim->image.dim_2d_max);
    printf("\nimage - max 3d: %u", lim->image.dim_3d_max);
    printf("\nimage - max layers: %u", lim->image.layer_max);

    printf("\ndtable - max per stage resources: %u", lim->dtable.stg_res_max);
    printf("\ndtable - max uniforms buffers: %u", lim->dtable.unif_max);
    printf("\ndtable - max mutables buffers: %u", lim->dtable.mut_max);
    printf("\ndtable - max r/w images: %u", lim->dtable.img_max);
    printf("\ndtable - max sampled images: %u", lim->dtable.sampd_max);
    printf("\ndtable - max samplers: %u", lim->dtable.sampr_max);
    printf("\ndtable - max image+samplers: %u", lim->dtable.isamp_max);
    printf("\ndtable - max copy size (uniform): %lu",
           lim->dtable.cpy_unif_sz_max);
    printf("\ndtable - max copy size (mutable): %lu",
           lim->dtable.cpy_mut_sz_max);

    printf("\nvinput - max input attributes: %u", lim->vinput.attr_max);
    printf("\nvinput - max input offset: %u", lim->vinput.off_max);
    printf("\nvinput - max input stride: %u", lim->vinput.strd_max);

    printf("\npass - max color attachments: %u", lim->pass.color_max);
    printf("\npass - max target dimensions: %u, %u", lim->pass.dim_max.width,
           lim->pass.dim_max.height);
    printf("\npass - max target layers: %u", lim->pass.layer_max);

    printf("\nviewport - max: %u", lim->viewport.max);
    printf("\nviewport - max dimensions: %u, %u", lim->viewport.dim_max.width,
           lim->viewport.dim_max.height);
    printf("\nviewport - min/max bounds: %.3f, %.3f", lim->viewport.bounds_min,
           lim->viewport.bounds_max);

    printf("\nstate - max bound dtables: %u", lim->state.dtable_max);
    printf("\nstate - max vinputs: %u", lim->state.vinput_max);

    printf("\ncmdbuf - max draw index value: %u", lim->cmdbuf.draw_idx_max);

    printf("\n\n");
}
