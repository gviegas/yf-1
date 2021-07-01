/*
 * YF
 * limits.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"

#include "yf-limits.h"
#include "context.h"

/* Destroys the 'YF_limits' data stored in a given context. */
static void destroy_lim(YF_context ctx);

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
    const VkFlags req_mem =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
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

    lim->dtable.stg_res_max = dl->maxPerStageResources;
    lim->dtable.unif_max = dl->maxPerStageDescriptorUniformBuffers;
    lim->dtable.mut_max = dl->maxPerStageDescriptorStorageBuffers;
    lim->dtable.img_max = dl->maxPerStageDescriptorStorageImages;
    lim->dtable.sampd_max = dl->maxPerStageDescriptorSampledImages;
    lim->dtable.sampr_max = dl->maxPerStageDescriptorSamplers;
    lim->dtable.isamp_max = YF_MIN(lim->dtable.sampd_max,
                                   lim->dtable.sampr_max);
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

static void destroy_lim(YF_context ctx)
{
    assert(ctx != NULL);

    free(ctx->lim.priv);
    ctx->lim.priv = NULL;
}
