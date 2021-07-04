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
    for (unsigned i = 0; i < ctx->layer_n; i++)
        printf("\n\t%s", ctx->layers[i]);
    printf("\ninstance extensions: #%u", ctx->inst_ext_n);
    for (unsigned i = 0; i < ctx->inst_ext_n; i++)
        printf("\n\t%s", ctx->inst_exts[i]);
    printf("\ndevice extensions: #%u", ctx->dev_ext_n);
    for (unsigned i = 0; i < ctx->dev_ext_n; i++)
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
    printf("\nimage - sample mask (color): %u", lim->image.sample_mask_clr);
    printf("\nimage - sample mask (depth): %u", lim->image.sample_mask_dep);
    printf("\nimage - sample mask (stencil): %u", lim->image.sample_mask_sten);
    printf("\nimage - sample mask (s. image): %u", lim->image.sample_mask_img);

    printf("\ndtable - max per stage resources: %u", lim->dtable.stg_res_max);
    printf("\ndtable - max uniform buffers: %u", lim->dtable.unif_max);
    printf("\ndtable - max mutable buffers: %u", lim->dtable.mut_max);
    printf("\ndtable - max r/w images: %u", lim->dtable.img_max);
    printf("\ndtable - max sampled images: %u", lim->dtable.spld_max);
    printf("\ndtable - max samplers: %u", lim->dtable.splr_max);
    printf("\ndtable - max image+samplers: %u", lim->dtable.ispl_max);
    printf("\ndtable - min copy alignment (uniform): %lu",
           lim->dtable.cpy_unif_align_min);
    printf("\ndtable - max copy size (uniform): %lu",
           lim->dtable.cpy_unif_sz_max);
    printf("\ndtable - min copy alignment (mutable): %lu",
           lim->dtable.cpy_mut_align_min);
    printf("\ndtable - max copy size (mutable): %lu",
           lim->dtable.cpy_mut_sz_max);

    printf("\nvinput - max input attributes: %u", lim->vinput.attr_max);
    printf("\nvinput - max input offset: %u", lim->vinput.off_max);
    printf("\nvinput - max input stride: %u", lim->vinput.strd_max);

    printf("\npass - max color attachments: %u", lim->pass.color_max);
    printf("\npass - max target dimensions: %u, %u", lim->pass.dim_max.width,
           lim->pass.dim_max.height);
    printf("\npass - max target layers: %u", lim->pass.layer_max);
    printf("\npass - sample mask (color): %u", lim->pass.sample_mask_clr);
    printf("\npass - sample mask (depth): %u", lim->pass.sample_mask_dep);
    printf("\npass - sample mask (stencil): %u", lim->pass.sample_mask_sten);

    printf("\nviewport - max: %u", lim->viewport.max);
    printf("\nviewport - max dimensions: %u, %u", lim->viewport.dim_max.width,
           lim->viewport.dim_max.height);
    printf("\nviewport - min bounds: %.3f", lim->viewport.bounds_min);
    printf("\nviewport - max bounds: %.3f", lim->viewport.bounds_max);

    printf("\nstate - max bound dtables: %u", lim->state.dtable_max);
    printf("\nstate - max vinputs: %u", lim->state.vinput_max);

    printf("\nshader - max vertex output components: %u",
           lim->shader.vert_out_max);
    printf("\nshader - max fragment input components: %u",
           lim->shader.frag_in_max);
    printf("\nshader - min point size: %.3f", lim->shader.point_sz_min);
    printf("\nshader - max point size: %.3f", lim->shader.point_sz_max);
    printf("\nshader - point size granularity: %.3f",
           lim->shader.point_sz_gran);
    printf("\nshader - min line width: %.3f", lim->shader.line_wdt_min);
    printf("\nshader - max line width: %.3f", lim->shader.line_wdt_max);
    printf("\nshader - line width granularity: %.3f",
           lim->shader.line_wdt_gran);

    printf("\ncmdbuf - max draw index value: %u", lim->cmdbuf.draw_idx_max);
    printf("\ncmdbuf - max dispatch work group dimensions: %u, %u, %u",
           lim->cmdbuf.disp_dim_max.width, lim->cmdbuf.disp_dim_max.height,
           lim->cmdbuf.disp_dim_max.depth);

    printf("\n\n");
}
