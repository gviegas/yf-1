/*
 * YF
 * limits.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#ifdef YF_DEVEL
# include <stdio.h>
#endif

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"

#include "yf-limits.h"
#include "context.h"

/* Destroys the 'YF_limits' data stored in a context. */
static void destroy_lim(YF_context ctx)
{
    assert(ctx != NULL);

    free(ctx->lim.priv);
    ctx->lim.priv = NULL;
}

const YF_limits *yf_getlimits(YF_context ctx)
{
    assert(ctx != NULL);

    if (ctx->lim.priv != NULL)
        return (const YF_limits *)ctx->lim.priv;

    YF_limits *lim = malloc(sizeof(YF_limits));
    if (lim == NULL)
        /* XXX */
        abort();

    const VkPhysicalDeviceLimits *dl = &ctx->dev_prop.limits;
    const VkFlags req_mem = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const VkFlags opt_mem = req_mem | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    lim->memory.obj_max = dl->maxMemoryAllocationCount;

    for (unsigned i = 0; i < ctx->mem_prop.memoryTypeCount; i++) {
        if ((ctx->mem_prop.memoryTypes[i].propertyFlags & opt_mem) == opt_mem) {
            unsigned j = ctx->mem_prop.memoryTypes[i].heapIndex;
            lim->buffer.sz_max = ctx->mem_prop.memoryHeaps[j].size;
            break;
        }

        if ((ctx->mem_prop.memoryTypes[i].propertyFlags & req_mem) == req_mem) {
            if (lim->buffer.sz_max != 0) {
                unsigned j = ctx->mem_prop.memoryTypes[i].heapIndex;
                lim->buffer.sz_max = ctx->mem_prop.memoryHeaps[j].size;
            }
        }
    }

    lim->image.dim_1d_max = dl->maxImageDimension1D;
    lim->image.dim_2d_max = dl->maxImageDimension2D;
    lim->image.dim_3d_max = dl->maxImageDimension3D;
    lim->image.layer_max = dl->maxImageArrayLayers;
    lim->image.sample_mask_clr = dl->sampledImageColorSampleCounts & 0x7f;
    lim->image.sample_mask_dep = dl->sampledImageDepthSampleCounts & 0x7f;
    lim->image.sample_mask_sten = dl->sampledImageStencilSampleCounts & 0x7f;
    lim->image.sample_mask_img = dl->storageImageSampleCounts & 0x7f;

    lim->dtable.stg_res_max = dl->maxPerStageResources;
    lim->dtable.unif_max = dl->maxPerStageDescriptorUniformBuffers;
    lim->dtable.mut_max = dl->maxPerStageDescriptorStorageBuffers;
    lim->dtable.img_max = dl->maxPerStageDescriptorStorageImages;
    lim->dtable.spld_max = dl->maxPerStageDescriptorSampledImages;
    lim->dtable.splr_max = dl->maxPerStageDescriptorSamplers;
    lim->dtable.ispl_max = YF_MIN(lim->dtable.spld_max, lim->dtable.splr_max);
    lim->dtable.cpy_unif_align_min = dl->minUniformBufferOffsetAlignment;
    lim->dtable.cpy_unif_sz_max = dl->maxUniformBufferRange;
    lim->dtable.cpy_mut_align_min = dl->minStorageBufferOffsetAlignment;
    lim->dtable.cpy_mut_sz_max = dl->maxStorageBufferRange;

    lim->vinput.attr_max = dl->maxVertexInputAttributes;
    lim->vinput.off_max = dl->maxVertexInputAttributeOffset;
    lim->vinput.strd_max = dl->maxVertexInputBindingStride;

    lim->pass.color_max = dl->maxColorAttachments;
    lim->pass.dim_max.width = dl->maxFramebufferWidth;
    lim->pass.dim_max.height = dl->maxFramebufferHeight;
    lim->pass.layer_max = dl->maxFramebufferLayers;
    lim->pass.sample_mask_clr = dl->framebufferColorSampleCounts & 0x7f;
    lim->pass.sample_mask_dep = dl->framebufferDepthSampleCounts & 0x7f;
    lim->pass.sample_mask_sten = dl->framebufferStencilSampleCounts & 0x7f;

    lim->viewport.max = dl->maxViewports;
    lim->viewport.dim_max.width = dl->maxViewportDimensions[0];
    lim->viewport.dim_max.height = dl->maxViewportDimensions[1];
    lim->viewport.bounds_min = dl->viewportBoundsRange[0];
    lim->viewport.bounds_max = dl->viewportBoundsRange[1];

    lim->state.dtable_max = dl->maxBoundDescriptorSets;
    lim->state.vinput_max = dl->maxVertexInputBindings;

    lim->shader.vert_out_max = dl->maxVertexOutputComponents;
    lim->shader.frag_in_max = dl->maxFragmentInputComponents;
    lim->shader.point_sz_min = dl->pointSizeRange[0];
    lim->shader.point_sz_max = dl->pointSizeRange[1];
    lim->shader.point_sz_gran = dl->pointSizeGranularity;
    lim->shader.line_wdt_min = dl->lineWidthRange[0];
    lim->shader.line_wdt_max = dl->lineWidthRange[1];
    lim->shader.line_wdt_gran = dl->lineWidthGranularity;

    lim->cmdbuf.draw_idx_max = dl->maxDrawIndexedIndexValue;
    lim->cmdbuf.disp_dim_max.width = dl->maxComputeWorkGroupCount[0];
    lim->cmdbuf.disp_dim_max.height = dl->maxComputeWorkGroupCount[1];
    lim->cmdbuf.disp_dim_max.depth = dl->maxComputeWorkGroupCount[2];

    ctx->lim.priv = lim;
    ctx->lim.deinit_callb = destroy_lim;
    return lim;
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

void yf_print_lim(const YF_limits *lim)
{
    assert(lim != NULL);

    printf("\n[YF] OUTPUT (%s):\n", __func__);

    printf(" limits:\n"
           "  memory:\n"
           "   max objects: %zu\n"
           "  buffer:\n"
           "   max size: %zu\n",
           lim->memory.obj_max, lim->buffer.sz_max);

    printf("  image:\n"
           "   max 1D:     %u\n"
           "   max 2D:     %u\n"
           "   max 3D:     %u\n"
           "   max layers: %u\n"
           "   sample mask:\n"
           "    color:    %u\n"
           "    depth:    %u\n"
           "    stencil:  %u\n"
           "    s. image: %u\n",
           lim->image.dim_1d_max, lim->image.dim_2d_max, lim->image.dim_3d_max,
           lim->image.layer_max, lim->image.sample_mask_clr,
           lim->image.sample_mask_dep, lim->image.sample_mask_sten,
           lim->image.sample_mask_img);

    printf("  dtable:\n"
           "   max per stage res.:    %u\n"
           "   max unif. buffers:     %u\n"
           "   max mut. buffers:      %u\n"
           "   max r/w images:        %u\n"
           "   max sampled images:    %u\n"
           "   max samplers:          %u\n"
           "   max combined img/splr: %u\n"
           "   copy:\n"
           "    min alignement (unif): %zu\n"
           "    max size (unif):       %zu\n"
           "    min alignement (mut):  %zu\n"
           "    max size (mut):        %zu\n",
           lim->dtable.stg_res_max, lim->dtable.unif_max, lim->dtable.mut_max,
           lim->dtable.img_max, lim->dtable.spld_max, lim->dtable.splr_max,
           lim->dtable.ispl_max, lim->dtable.cpy_unif_align_min,
           lim->dtable.cpy_unif_sz_max, lim->dtable.cpy_mut_align_min,
           lim->dtable.cpy_mut_sz_max);

    printf("  vinput:\n"
           "   max attributes: %u\n"
           "   max offset:     %u\n"
           "   max stride:     %u\n",
           lim->vinput.attr_max, lim->vinput.off_max, lim->vinput.strd_max);

    printf("  pass:\n"
           "   max colors:     %u\n"
           "   max dimensions: %ux%u\n"
           "   max layers:     %u\n"
           "   sample mask:\n"
           "    color:   %u\n"
           "    depth:   %u\n"
           "    stencil: %u\n",
           lim->pass.color_max, lim->pass.dim_max.width,
           lim->pass.dim_max.height, lim->pass.layer_max,
           lim->pass.sample_mask_clr, lim->pass.sample_mask_dep,
           lim->pass.sample_mask_sten);

    printf("  viewport:\n"
           "   max:            %u\n"
           "   max dimensions: %ux%u\n"
           "   min bounds:     %.6f\n"
           "   max bounds:     %.6f\n",
           lim->viewport.max, lim->viewport.dim_max.width,
           lim->viewport.dim_max.height, lim->viewport.bounds_min,
           lim->viewport.bounds_max);

    printf("  state:\n"
           "   max bound dtables: %u\n"
           "   max vinputs:       %u\n",
           lim->state.dtable_max, lim->state.vinput_max);

    printf("  shader:\n"
           "   max vert. output components: %u\n"
           "   max frag. input components:  %u\n"
           "   min point size:              %.6f\n"
           "   max point size:              %.6f\n"
           "   point size granularity:      %.6f\n"
           "   min line width:              %.6f\n"
           "   max line width:              %.6f\n"
           "   line width granularity:      %.6f\n",
           lim->shader.vert_out_max, lim->shader.frag_in_max,
           lim->shader.point_sz_min, lim->shader.point_sz_max,
           lim->shader.point_sz_gran, lim->shader.line_wdt_min,
           lim->shader.line_wdt_max, lim->shader.line_wdt_gran);

    printf("  cmdbuf:\n"
           "   max draw index value:         %u\n"
           "   max dispatch work group dim.: %ux%ux%u\n",
           lim->cmdbuf.draw_idx_max, lim->cmdbuf.disp_dim_max.width,
           lim->cmdbuf.disp_dim_max.height, lim->cmdbuf.disp_dim_max.depth);

    puts("");
}

#endif
