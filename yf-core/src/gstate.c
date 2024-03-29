/*
 * YF
 * gstate.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "gstate.h"
#include "context.h"
#include "pass.h"
#include "stage.h"
#include "dtable.h"
#include "vinput.h"
#include "yf-limits.h"

yf_gstate_t *yf_gstate_init(yf_context_t *ctx, const yf_gconf_t *conf)
{
    assert(ctx != NULL);
    assert(conf != NULL);

    if (conf->pass == NULL || conf->stg_n == 0) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    const yf_limits_t *lim = yf_getlimits(ctx);

    if (conf->dtb_n > lim->state.dtable_max ||
        conf->vin_n > lim->state.vinput_max) {
        yf_seterr(YF_ERR_LIMIT, __func__);
        return NULL;
    }

    yf_gstate_t *gst = calloc(1, sizeof(yf_gstate_t));
    if (gst == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    gst->ctx = ctx;
    gst->pass = conf->pass;

    const size_t stg_sz = conf->stg_n * sizeof *conf->stgs;
    gst->stgs = malloc(stg_sz);
    if (gst->stgs == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_gstate_deinit(gst);
        return NULL;
    }
    memcpy(gst->stgs, conf->stgs, stg_sz);
    gst->stg_n = conf->stg_n;

    gst->dtb_n = conf->dtb_n;
    if (gst->dtb_n > 0) {
        const size_t dtb_sz = conf->dtb_n * sizeof *conf->dtbs;
        gst->dtbs = malloc(dtb_sz);
        if (gst->dtbs == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            yf_gstate_deinit(gst);
            return NULL;
        }
        memcpy(gst->dtbs, conf->dtbs, dtb_sz);
    }

    VkPrimitiveTopology topol;
    VkPolygonMode polym;
    VkCullModeFlagBits cullm;
    VkFrontFace fface;
    YF_TOPOLOGY_FROM(conf->topology, topol);
    YF_POLYMODE_FROM(conf->polymode, polym);
    YF_CULLMODE_FROM(conf->cullmode, cullm);
    YF_WINDING_FROM(conf->winding, fface);

    if (topol == INT_MAX || polym == INT_MAX ||
        cullm == INT_MAX || fface == INT_MAX) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        yf_gstate_deinit(gst);
        return NULL;
    }

    VkResult res;

    /* layout */
    VkPipelineLayoutCreateInfo lay_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = conf->dtb_n,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    VkDescriptorSetLayout *ds_lays = NULL;
    if (conf->dtb_n > 0) {
        ds_lays = malloc(conf->dtb_n * sizeof *ds_lays);
        if (ds_lays == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            yf_gstate_deinit(gst);
            return NULL;
        }
        for (unsigned i = 0; i < conf->dtb_n; i++)
            ds_lays[i] = conf->dtbs[i]->layout;
        lay_info.pSetLayouts = ds_lays;
    }

    res = vkCreatePipelineLayout(ctx->device, &lay_info, NULL, &gst->layout);
    free(ds_lays);
    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        yf_gstate_deinit(gst);
        return NULL;
    }

    /* shader stage */
    VkPipelineShaderStageCreateInfo *ss;
    ss = malloc(conf->stg_n * sizeof *ss);
    if (ss == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_gstate_deinit(gst);
        return NULL;
    }

    unsigned stg_mask = 0;
    for (unsigned i = 0; i < conf->stg_n; i++) {
        VkShaderModule module = yf_getshd(ctx, conf->stgs[i].shd);
        if (module == VK_NULL_HANDLE ||
            !YF_STAGE_ONE(conf->stgs[i].stage) ||
            (conf->stgs[i].stage & stg_mask) != 0) {

            yf_seterr(YF_ERR_INVARG, __func__);
            yf_gstate_deinit(gst);
            free(ss);
            return NULL;
        }

        ss[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ss[i].pNext = NULL;
        ss[i].flags = 0;
        YF_STAGE_FROM(conf->stgs[i].stage, ss[i].stage);
        ss[i].module = module;

        gst->stgs[i].entry_point[(sizeof gst->stgs[i].entry_point) - 1] = '\0';
        ss[i].pName = gst->stgs[i].entry_point;

        ss[i].pSpecializationInfo = NULL;
        stg_mask |= conf->stgs[i].stage;
    }

    if (YF_STAGE_INVGRAPH(stg_mask)) {
        yf_seterr(YF_ERR_INVARG, __func__);
        yf_gstate_deinit(gst);
        free(ss);
        return NULL;
    }

    /* vertex input */
    VkPipelineVertexInputStateCreateInfo vi = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL
    };

    VkVertexInputBindingDescription *binds = NULL;
    VkVertexInputAttributeDescription *attrs = NULL;
    if (conf->vin_n > 0) {
        assert(conf->vins != NULL);

        unsigned attr_n = 0;
        for (unsigned i = 0; i < conf->vin_n; i++)
            attr_n += conf->vins[i].attr_n;

        if (attr_n > 0) {
            if (attr_n > lim->vinput.attr_max) {
                yf_seterr(YF_ERR_LIMIT, __func__);
                free(ss);
                return NULL;
            }

            binds = malloc(conf->vin_n * sizeof *binds);
            attrs = malloc(attr_n * sizeof *attrs);
            if (binds == NULL || attrs == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                free(attrs);
                free(binds);
                free(ss);
                return NULL;
            }

            const yf_vattr_t *vattr = NULL;
            unsigned k = 0;
            for (unsigned i = 0; i < conf->vin_n; i++) {
                if (conf->vins[i].stride > lim->vinput.strd_max) {
                    yf_seterr(YF_ERR_LIMIT, __func__);
                    free(attrs);
                    free(binds);
                    free(ss);
                    return NULL;
                }

                binds[i].binding = i;
                binds[i].stride = conf->vins[i].stride;
                YF_VRATE_FROM(conf->vins[i].vrate, binds[i].inputRate);

                assert(binds[i].inputRate != INT_MAX);

                for (unsigned j = 0; j < conf->vins[i].attr_n; j++, k++) {
                    vattr = conf->vins[i].attrs+j;

                    if (vattr->offset > lim->vinput.off_max) {
                        yf_seterr(YF_ERR_LIMIT, __func__);
                        free(attrs);
                        free(binds);
                        free(ss);
                        return NULL;
                    }

                    attrs[k].location = vattr->location;
                    attrs[k].binding = i;
                    YF_VFMT_FROM(vattr->vfmt, attrs[k].format);

                    assert(attrs[k].format != VK_FORMAT_UNDEFINED);

                    attrs[k].offset = vattr->offset;
                }
            }

            vi.vertexBindingDescriptionCount = conf->vin_n;
            vi.pVertexBindingDescriptions = binds;
            vi.vertexAttributeDescriptionCount = attr_n;
            vi.pVertexAttributeDescriptions = attrs;
        }
    }

    /* input assembly */
    VkPipelineInputAssemblyStateCreateInfo ia = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .topology = topol,
        .primitiveRestartEnable = VK_FALSE,
    };

    /* viewport */
    VkPipelineViewportStateCreateInfo vp = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = NULL,
        .scissorCount = 1,
        .pScissors = NULL
    };

    /* rasterization */
    VkPipelineRasterizationStateCreateInfo rz = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = polym,
        .cullMode = cullm,
        .frontFace = fface,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    /* multisample */
    VkPipelineMultisampleStateCreateInfo ms = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    /* depth/stencil */
    VkPipelineDepthStencilStateCreateInfo ds = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .compareMask = 0,
            .writeMask = 0,
            .reference = 0
        },
        .back = {
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .compareMask = 0,
            .writeMask = 0,
            .reference = 0
        },
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 0.0f
    };

    /* color blend */
    VkPipelineColorBlendAttachmentState *cb_atts = NULL;
    if (conf->pass->color_n > 0) {
        cb_atts = malloc(conf->pass->color_n * sizeof *cb_atts);
        if (cb_atts == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            yf_gstate_deinit(gst);
            free(attrs);
            free(binds);
            free(ss);
            return NULL;
        }

        cb_atts[0].blendEnable = VK_TRUE;
        cb_atts[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cb_atts[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        cb_atts[0].colorBlendOp = VK_BLEND_OP_ADD;
        cb_atts[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        cb_atts[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cb_atts[0].alphaBlendOp = VK_BLEND_OP_ADD;
        cb_atts[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                    VK_COLOR_COMPONENT_G_BIT |
                                    VK_COLOR_COMPONENT_B_BIT |
                                    VK_COLOR_COMPONENT_A_BIT;

        for (unsigned i = 1; i < conf->pass->color_n; i++)
            memcpy(&cb_atts[i], &cb_atts[0], sizeof cb_atts[0]);
    }

    VkPipelineColorBlendStateCreateInfo cb = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = conf->pass->color_n,
        .pAttachments = cb_atts,
        .blendConstants = {1.0f, 1.0f, 1.0f, 1.0f}
    };

    /* dynamic */
    VkDynamicState dy_vals[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
            /* TODO: Remaining dynamic states. */
    };
    VkPipelineDynamicStateCreateInfo dy = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .dynamicStateCount = sizeof dy_vals / sizeof dy_vals[0],
        .pDynamicStates = dy_vals
    };

    /* pipeline */
    VkGraphicsPipelineCreateInfo pl_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stageCount = conf->stg_n,
        .pStages = ss,
        .pVertexInputState = &vi,
        .pInputAssemblyState = &ia,
        .pTessellationState = NULL, /* TODO */
        .pViewportState = &vp,
        .pRasterizationState = &rz,
        .pMultisampleState = &ms,
        .pDepthStencilState = &ds,
        .pColorBlendState = &cb,
        .pDynamicState = &dy,
        .layout = gst->layout,
        .renderPass = conf->pass->ren_pass,
        .basePipelineHandle = NULL,
        .basePipelineIndex = -1
    };

    res = vkCreateGraphicsPipelines(ctx->device, ctx->pl_cache, 1, &pl_info,
                                    NULL, &gst->pipeline);
    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        yf_gstate_deinit(gst);
        gst = NULL;
    }

    free(cb_atts);
    free(attrs);
    free(binds);
    free(ss);
    return gst;
}

yf_pass_t *yf_gstate_getpass(yf_gstate_t *gst)
{
    assert(gst != NULL);
    return gst->pass;
}

const yf_stage_t *yf_gstate_getstg(yf_gstate_t *gst, int stage)
{
    assert(gst != NULL);

    for (unsigned i = 0; i < gst->stg_n; i++) {
        if (gst->stgs[i].stage == stage)
            return gst->stgs+i;
    }

    return NULL;
}

yf_dtable_t *yf_gstate_getdtb(yf_gstate_t *gst, unsigned index)
{
    assert(gst != NULL);

    if (index >= gst->dtb_n) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    return gst->dtbs[index];
}

void yf_gstate_deinit(yf_gstate_t *gst)
{
    if (gst != NULL) {
        free(gst->stgs);
        free(gst->dtbs);
        vkDestroyPipelineLayout(gst->ctx->device, gst->layout, NULL);
        vkDestroyPipeline(gst->ctx->device, gst->pipeline, NULL);
        free(gst);
    }
}
