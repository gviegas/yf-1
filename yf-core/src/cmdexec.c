/*
 * YF
 * cmdexec.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-util.h"
#include "yf/com/yf-list.h"
#include "yf/com/yf-error.h"

#include "cmdexec.h"
#include "context.h"
#include "cmdbuf.h"

/* TODO: Should be defined elsewhere. */
#define YF_CMDEMIN 1
#define YF_CMDEMAX 32

#define YF_CMDEWAIT 16666666UL

/* Queue entry. */
typedef struct {
    YF_cmdres cmdr;
    void (*callb)(int, void *);
    void *arg;
} T_entry;

/* Execution queue. */
typedef struct {
    T_entry *entries;
    VkCommandBuffer *buffers;
    unsigned n;
    unsigned cap;
    VkSubmitInfo subm_info;
} T_cmde;

/* Submission state. */
typedef struct {
    VkFence fence;
    VkSemaphore prio_sem;
    VkPipelineStageFlags prio_stg;
    YF_list wait_sems;
    YF_list wait_stgs;
} T_subm;

/* Execution queues stored in a context. */
typedef struct {
    T_cmde cmde;
    T_cmde prio;
    T_subm subm;
} T_priv;

/* Initializes a pre-allocated queue. */
static int init_queue(YF_context ctx, T_cmde *cmde)
{
    assert(ctx != NULL);
    assert(cmde != NULL);
    assert(cmde->cap > 0);

    cmde->entries = malloc(sizeof(T_entry) * cmde->cap);
    cmde->buffers = malloc(sizeof(VkCommandBuffer) * cmde->cap);
    if (cmde->entries == NULL || cmde->buffers == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    cmde->subm_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    cmde->subm_info.pNext = NULL;
    cmde->subm_info.waitSemaphoreCount = 0;
    cmde->subm_info.pWaitSemaphores = NULL;
    cmde->subm_info.pWaitDstStageMask = NULL;
    cmde->subm_info.commandBufferCount = 0;
    cmde->subm_info.pCommandBuffers = cmde->buffers;
    cmde->subm_info.signalSemaphoreCount = 0;
    cmde->subm_info.pSignalSemaphores = NULL;

    cmde->n = 0;
    return 0;
}

/* Enqueues commands in a queue. */
static int enqueue_res(T_cmde *cmde, const YF_cmdres *cmdr,
                       void (*callb)(int res, void *arg), void *arg)
{
    assert(cmde != NULL);
    assert(cmdr != NULL);

    if (cmde->n == cmde->cap) {
        yf_seterr(YF_ERR_QFULL, __func__);
        return -1;
    }

    memcpy(&cmde->entries[cmde->n].cmdr, cmdr, sizeof *cmdr);
    cmde->entries[cmde->n].callb = callb;
    cmde->entries[cmde->n].arg = arg;
    cmde->buffers[cmde->n] = cmdr->pool_res;
    cmde->n++;

    return 0;
}

/* Ends priority queue and enqueues its resources. */
static int end_prio(YF_context ctx, T_cmde *prio)
{
    assert(ctx != NULL);
    assert(prio != NULL);

    int r = 0;

    const YF_cmdres *cmdr_list;
    unsigned cmdr_n;
    yf_cmdpool_checkprio(ctx, &cmdr_list, &cmdr_n);

    if (cmdr_n == 0)
        return r;

    for (unsigned i = 0; i < cmdr_n; i++) {
        if (vkEndCommandBuffer(cmdr_list[i].pool_res) != VK_SUCCESS) {
            yf_seterr(YF_ERR_DEVGEN, __func__);
            r = -1;
            break;
        }
        if (enqueue_res(prio, cmdr_list+i, NULL, NULL) != 0) {
            r = -1;
            break;
        }
    }
    return r;
}

/* Executes a command queue. */
static int exec_queue(YF_context ctx, T_cmde *cmde, T_subm *subm)
{
    assert(ctx != NULL);
    assert(cmde != NULL);
    assert(subm != NULL);

    if (cmde->n < 1)
        return 0;

    int r = 0;
    VkResult res;

    cmde->subm_info.commandBufferCount = cmde->n;
    cmde->subm_info.signalSemaphoreCount = 0;
    cmde->subm_info.pSignalSemaphores = NULL;

    const unsigned sem_n = yf_list_getlen(subm->wait_sems);

    if (sem_n > 0) {
        VkSemaphore sems[sem_n];
        VkPipelineStageFlags stgs[sem_n];
        for (unsigned i = 0; i < sem_n; i++) {
            sems[i] = yf_list_removeat(subm->wait_sems, NULL);
            stgs[i] = (uintptr_t)yf_list_removeat(subm->wait_stgs, NULL);
        }
        cmde->subm_info.waitSemaphoreCount = sem_n;
        cmde->subm_info.pWaitSemaphores = sems;
        cmde->subm_info.pWaitDstStageMask = stgs;
        res = vkQueueSubmit(ctx->queue, 1, &cmde->subm_info, subm->fence);
    } else {
        cmde->subm_info.waitSemaphoreCount = 0;
        cmde->subm_info.pWaitSemaphores = NULL;
        cmde->subm_info.pWaitDstStageMask = NULL;
        res = vkQueueSubmit(ctx->queue, 1, &cmde->subm_info, subm->fence);
    }

    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        r = -1;
    } else {
        while ((res = vkWaitForFences(ctx->device, 1, &subm->fence, VK_TRUE,
                                      YF_CMDEWAIT)) == VK_TIMEOUT)
            ;
        if (res != VK_SUCCESS) {
            yf_seterr(YF_ERR_DEVGEN, __func__);
            r = -1;
        }
    }

    for (unsigned i = 0; i < cmde->n; i++) {
        yf_cmdpool_yield(ctx, &cmde->entries[i].cmdr);
        if (cmde->entries[i].callb != NULL)
            cmde->entries[i].callb(r, cmde->entries[i].arg);
    }
    cmde->n = 0;

    return r;
}

/* Executes priority and non-priority command queues. */
static int exec_queues(YF_context ctx, T_cmde *prio, T_cmde *cmde,
                       T_subm *subm)
{
    assert(ctx != NULL);
    assert(prio != NULL);
    assert(cmde != NULL);
    assert(subm != NULL);

    if (prio->n < 1)
        return exec_queue(ctx, cmde, subm);

    if (cmde->n < 1)
        return exec_queue(ctx, prio, subm);

    int r = 0;
    VkResult res;

    if (subm->prio_stg == 0)
        subm->prio_stg = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    prio->subm_info.commandBufferCount = prio->n;
    prio->subm_info.signalSemaphoreCount = 1;
    prio->subm_info.pSignalSemaphores = &subm->prio_sem;

    cmde->subm_info.waitSemaphoreCount = 1;
    cmde->subm_info.pWaitSemaphores = &subm->prio_sem;
    cmde->subm_info.pWaitDstStageMask = &subm->prio_stg;
    cmde->subm_info.commandBufferCount = cmde->n;
    cmde->subm_info.signalSemaphoreCount = 0;
    cmde->subm_info.pSignalSemaphores = NULL;

    const unsigned sem_n = yf_list_getlen(subm->wait_sems);

    if (sem_n > 0) {
        VkSemaphore sems[sem_n];
        VkPipelineStageFlags stgs[sem_n];
        for (unsigned i = 0; i < sem_n; i++) {
            sems[i] = yf_list_removeat(subm->wait_sems, NULL);
            stgs[i] = (uintptr_t)yf_list_removeat(subm->wait_stgs, NULL);
        }
        prio->subm_info.waitSemaphoreCount = sem_n;
        prio->subm_info.pWaitSemaphores = sems;
        prio->subm_info.pWaitDstStageMask = stgs;
        const VkSubmitInfo subm_infos[2] = {prio->subm_info, cmde->subm_info};
        res = vkQueueSubmit(ctx->queue, 2, subm_infos, subm->fence);
    } else {
        prio->subm_info.waitSemaphoreCount = 0;
        prio->subm_info.pWaitSemaphores = NULL;
        prio->subm_info.pWaitDstStageMask = 0;
        const VkSubmitInfo subm_infos[2] = {prio->subm_info, cmde->subm_info};
        res = vkQueueSubmit(ctx->queue, 2, subm_infos, subm->fence);
    }

    subm->prio_stg = 0;

    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        r = -1;
    } else {
        while ((res = vkWaitForFences(ctx->device, 1, &subm->fence, VK_TRUE,
                                      YF_CMDEWAIT)) == VK_TIMEOUT)
            ;
        if (res != VK_SUCCESS) {
            yf_seterr(YF_ERR_DEVGEN, __func__);
            r = -1;
        }
    }

    T_cmde *cmdes[2] = {prio, cmde};

    for (unsigned i = 0; i < 2; i++) {
        for (unsigned j = 0; j < cmdes[i]->n; j++) {
            yf_cmdpool_yield(ctx, &cmdes[i]->entries[j].cmdr);
            if (cmdes[i]->entries[j].callb != NULL)
                cmdes[i]->entries[j].callb(r, cmdes[i]->entries[j].arg);
        }
        cmdes[i]->n = 0;
    }

    return r;
}

/* Resets a command queue. */
static void reset_queue(YF_context ctx, T_cmde *cmde)
{
    assert(ctx != NULL);
    assert(cmde != NULL);

    if (cmde->n < 1)
        return;

    for (unsigned i = 0; i < cmde->n; i++) {
        yf_cmdpool_reset(ctx, &cmde->entries[i].cmdr);
        if (cmde->entries[i].callb != NULL)
            cmde->entries[i].callb(-1, cmde->entries[i].arg);
    }
    cmde->n = 0;
}

/* Deinitializes and deallocates a queue. */
static void deinit_queue(YF_context ctx, T_cmde *cmde)
{
    assert(ctx != NULL);

    if (cmde == NULL)
        return;

    if (cmde == &((T_priv *)ctx->cmde.priv)->prio)
        yf_cmdpool_notifyprio(ctx, -1);

    reset_queue(ctx, cmde);
    free(cmde->buffers);
    free(cmde->entries);

    /* XXX: The 'cmde' itself is not freed. */
}

/* Destroys the 'T_priv' data stored in a given context. */
static void destroy_priv(YF_context ctx)
{
    assert(ctx != NULL);

    if (ctx->cmde.priv == NULL)
        return;

    T_priv *priv = ctx->cmde.priv;

    deinit_queue(ctx, &priv->cmde);
    deinit_queue(ctx, &priv->prio);
    vkDestroyFence(ctx->device, priv->subm.fence, NULL);
    vkDestroySemaphore(ctx->device, priv->subm.prio_sem, NULL);

    if (priv->subm.wait_sems != NULL) {
        while (yf_list_getlen(priv->subm.wait_sems) > 0)
            vkDestroySemaphore(ctx->device,
                               yf_list_removeat(priv->subm.wait_sems, NULL),
                               NULL);
        yf_list_deinit(priv->subm.wait_sems);
    }
    yf_list_deinit(priv->subm.wait_stgs);

    free(priv);
    ctx->cmde.priv = NULL;
}

int yf_cmdexec_create(YF_context ctx, unsigned capacity)
{
    assert(ctx != NULL);

    if (ctx->cmde.priv != NULL)
        destroy_priv(ctx);

    T_priv *priv = calloc(1, sizeof(T_priv));
    if (priv == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    ctx->cmde.priv = priv;
    ctx->cmde.deinit_callb = destroy_priv;

    priv->cmde.cap = YF_CLAMP(capacity, YF_CMDEMIN, YF_CMDEMAX);
    priv->prio.cap = YF_CMDEMIN;

    if (init_queue(ctx, &priv->cmde) != 0 ||
        init_queue(ctx, &priv->prio) != 0) {
        destroy_priv(ctx);
        return -1;
    }

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };
    VkSemaphoreCreateInfo sem_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };

    if (vkCreateFence(ctx->device, &fence_info, NULL,
                      &priv->subm.fence) != VK_SUCCESS ||
        vkCreateSemaphore(ctx->device, &sem_info, NULL,
                          &priv->subm.prio_sem) != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        destroy_priv(ctx);
        return -1;
    }

    if ((priv->subm.wait_sems = yf_list_init(NULL)) == NULL ||
        (priv->subm.wait_stgs = yf_list_init(NULL)) == NULL) {
        destroy_priv(ctx);
        return -1;
    }

    return 0;
}

int yf_cmdexec_enqueue(YF_context ctx, const YF_cmdres *cmdr,
                       void (*callb)(int res, void *arg), void *arg)
{
    assert(ctx != NULL);
    assert(cmdr != NULL);
    assert(ctx->cmde.priv != NULL);

    return enqueue_res(&((T_priv *)ctx->cmde.priv)->cmde, cmdr, callb, arg);
}

int yf_cmdexec_exec(YF_context ctx)
{
    assert(ctx != NULL);
    assert(ctx->cmde.priv != NULL);

    T_priv *priv = ctx->cmde.priv;
    int r = 0;

    r = end_prio(ctx, &priv->prio);
    if (r == 0) {
        r = exec_queues(ctx, &priv->prio, &priv->cmde, &priv->subm);
    } else {
        reset_queue(ctx, &priv->prio);
        reset_queue(ctx, &priv->cmde);
    }

    yf_cmdpool_notifyprio(ctx, r);
    return r;
}

int yf_cmdexec_execprio(YF_context ctx)
{
    assert(ctx != NULL);
    assert(ctx->cmde.priv != NULL);

    T_priv *priv = ctx->cmde.priv;
    int r = 0;

    r = end_prio(ctx, &priv->prio);
    if (r == 0)
        r = exec_queue(ctx, &priv->prio, &priv->subm);
    else
        reset_queue(ctx, &priv->prio);

    yf_cmdpool_notifyprio(ctx, r);
    return r;
}

void yf_cmdexec_reset(YF_context ctx)
{
    assert(ctx != NULL);
    assert(ctx->cmde.priv != NULL);

    reset_queue(ctx, &((T_priv *)ctx->cmde.priv)->cmde);
}

void yf_cmdexec_resetprio(YF_context ctx)
{
    assert(ctx != NULL);
    assert(ctx->cmde.priv != NULL);

    reset_queue(ctx, &((T_priv *)ctx->cmde.priv)->prio);
    yf_cmdpool_notifyprio(ctx, -1);
}

void yf_cmdexec_waitfor(YF_context ctx, VkSemaphore sem,
                        VkPipelineStageFlags stg_mask)
{
    assert(ctx != NULL);
    assert(ctx->cmde.priv != NULL);

    YF_list wait_sems = ((T_priv *)ctx->cmde.priv)->subm.wait_sems;
    yf_list_insert(wait_sems, sem);

    YF_list wait_stgs = ((T_priv *)ctx->cmde.priv)->subm.wait_stgs;
    VkPipelineStageFlags sm;
    sm = stg_mask != 0 ? stg_mask : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    yf_list_insert(wait_stgs, (void *)(uintptr_t)sm);
}
