/*
 * YF
 * cstate.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "cstate.h"
#include "context.h"
#include "stage.h"
#include "dtable.h"
#include "yf-limits.h"

yf_cstate_t *yf_cstate_init(yf_context_t *ctx, const yf_cconf_t *conf)
{
    assert(ctx != NULL);
    assert(conf != NULL);

    if (conf->stg.stage != YF_STAGE_COMP) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    if (conf->dtb_n > yf_getlimits(ctx)->state.dtable_max) {
        yf_seterr(YF_ERR_LIMIT, __func__);
        return NULL;
    }

    yf_cstate_t *cst = calloc(1, sizeof(yf_cstate_t));
    if (cst == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    cst->ctx = ctx;

    memcpy(&cst->stg, &conf->stg, sizeof conf->stg);
    cst->stg.entry_point[(sizeof cst->stg.entry_point) - 1] = '\0';

    cst->dtb_n = conf->dtb_n;
    if (cst->dtb_n > 0) {
        const size_t dtb_sz = cst->dtb_n * sizeof *conf->dtbs;
        cst->dtbs = malloc(dtb_sz);
        if (cst->dtbs == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            yf_cstate_deinit(cst);
            return NULL;
        }
        memcpy(cst->dtbs, conf->dtbs, dtb_sz);
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
            yf_cstate_deinit(cst);
            return NULL;
        }
        for (unsigned i = 0; i < conf->dtb_n; i++)
            ds_lays[i] = conf->dtbs[i]->layout;
        lay_info.pSetLayouts = ds_lays;
    }

    res = vkCreatePipelineLayout(ctx->device, &lay_info, NULL, &cst->layout);
    free(ds_lays);
    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        yf_cstate_deinit(cst);
        return NULL;
    }

    /* pipeline */
    VkComputePipelineCreateInfo pl_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = yf_getshd(ctx, conf->stg.shd),
            .pName = cst->stg.entry_point,
            .pSpecializationInfo = NULL
        },
        .layout = cst->layout,
        .basePipelineHandle = NULL,
        .basePipelineIndex = -1
    };
    if (pl_info.stage.module == VK_NULL_HANDLE) {
        yf_seterr(YF_ERR_INVARG, __func__);
        yf_cstate_deinit(cst);
        return NULL;
    }

    res = vkCreateComputePipelines(ctx->device, ctx->pl_cache, 1, &pl_info,
                                   NULL, &cst->pipeline);
    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        yf_cstate_deinit(cst);
        cst = NULL;
    }

    return cst;
}

const yf_stage_t *yf_cstate_getstg(yf_cstate_t *cst)
{
    assert(cst != NULL);
    return &cst->stg;
}

yf_dtable_t *yf_cstate_getdtb(yf_cstate_t *cst, unsigned index)
{
    assert(cst != NULL);

    if (index >= cst->dtb_n) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    return cst->dtbs[index];
}

void yf_cstate_deinit(yf_cstate_t *cst)
{
    if (cst != NULL) {
        free(cst->dtbs);
        vkDestroyPipelineLayout(cst->ctx->device, cst->layout, NULL);
        vkDestroyPipeline(cst->ctx->device, cst->pipeline, NULL);
        free(cst);
    }
}
