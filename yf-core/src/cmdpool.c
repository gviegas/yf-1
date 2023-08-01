/*
 * YF
 * cmdpool.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-util.h"
#include "yf/com/yf-list.h"
#include "yf/com/yf-error.h"

#include "cmdpool.h"
#include "cmdbuf.h"
#include "context.h"

/* TODO: Should be defined elsewhere. */
#define YF_CMDPMIN 1
#define YF_CMDPMAX 32

/* Pool resource. */
typedef struct {
    VkCommandPool pool;
    VkCommandBuffer buffer;
    int in_use;
} entry_t;

/* Command pool. */
typedef struct {
    entry_t *entries;
    unsigned last_i;
    unsigned cur_n;
    unsigned cap;
} cmdp_t;

/* Command pool variables stored in a context. */
typedef struct {
    cmdp_t cmdp;
    yf_cmdres_t prio;
    yf_list_t *callbs;
} priv_t;

/* Callback from priority resource acquisition. */
typedef struct {
    void (*callb)(int, void *);
    void *arg;
} callb_t;

/* Initializes the pool entries. */
static int init_entries(yf_context_t *ctx, cmdp_t *cmdp)
{
    assert(ctx != NULL);
    assert(cmdp != NULL);

    cmdp->entries = calloc(cmdp->cap, sizeof(entry_t));
    if (cmdp->entries == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx->queue_i
    };
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = NULL,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VkResult res;

    for (unsigned i = 0; i < cmdp->cap; i++) {
        /* TODO: Consider using only one pool instead. */
        res = vkCreateCommandPool(ctx->device, &pool_info, NULL,
                                  &cmdp->entries[i].pool);
        if (res != VK_SUCCESS) {
            yf_seterr(YF_ERR_DEVGEN, __func__);
            return -1;
        }
        alloc_info.commandPool = cmdp->entries[i].pool;

        res = vkAllocateCommandBuffers(ctx->device, &alloc_info,
                                       &cmdp->entries[i].buffer);
        if (res != VK_SUCCESS) {
            yf_seterr(YF_ERR_DEVGEN, __func__);
            return -1;
        }
    }

    return 0;
}

/* Destroys the 'priv_t' data stored in a given context. */
static void destroy_priv(yf_context_t *ctx)
{
    assert(ctx != NULL);

    if (ctx->cmdp.priv == NULL)
        return;

    priv_t *priv = ctx->cmdp.priv;
    yf_iter_t it = YF_NILIT;
    do
        free(yf_list_next(priv->callbs, &it));
    while (!YF_IT_ISNIL(it));
    yf_list_deinit(priv->callbs);

    if (priv->cmdp.entries != NULL) {
        for (unsigned i = 0; i < priv->cmdp.cap; i++)
            vkDestroyCommandPool(ctx->device, priv->cmdp.entries[i].pool, NULL);
        free(priv->cmdp.entries);
    }

    free(priv);
    ctx->cmdp.priv = NULL;
}

int yf_cmdpool_create(yf_context_t *ctx, unsigned capacity)
{
    assert(ctx != NULL);

    if (ctx->cmdp.priv != NULL)
        destroy_priv(ctx);

    priv_t *priv = calloc(1, sizeof(priv_t));
    if (priv == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    ctx->cmdp.priv = priv;
    ctx->cmdp.deinit_callb = destroy_priv;

    priv->cmdp.last_i = 0;
    priv->cmdp.cur_n = 0;
    priv->cmdp.cap = YF_CLAMP(capacity, YF_CMDPMIN, YF_CMDPMAX);
    if (init_entries(ctx, &priv->cmdp) != 0) {
        destroy_priv(ctx);
        return  -1;
    }
    priv->prio.pool_res = NULL;
    priv->prio.res_id = -1;
    priv->callbs = yf_list_init(NULL);
    if (priv->callbs == NULL) {
        destroy_priv(ctx);
        return -1;
    }

    return 0;
}

int yf_cmdpool_obtain(yf_context_t *ctx, yf_cmdres_t *cmdr)
{
    assert(ctx != NULL);
    assert(cmdr != NULL);
    assert(ctx->cmdp.priv != NULL);

    cmdp_t *cmdp = &((priv_t *)ctx->cmdp.priv)->cmdp;
    if (cmdp->cur_n == cmdp->cap) {
        yf_seterr(YF_ERR_INUSE, __func__);
        return -1;
    }

    entry_t *e = NULL;
    for (unsigned i = 0; i < cmdp->cap; i++) {
        e = &cmdp->entries[cmdp->last_i];
        if (!e->in_use) {
            cmdr->res_id = cmdp->last_i;
            cmdr->pool_res = e->buffer;
            e->in_use = 1;
            cmdp->cur_n++;
            break;
        }
        cmdp->last_i = (cmdp->last_i + 1) % cmdp->cap;
        e = NULL;
    }
    if (e == NULL) {
        yf_seterr(YF_ERR_INUSE, __func__);
        return -1;
    }

    return 0;
}

void yf_cmdpool_yield(yf_context_t *ctx, yf_cmdres_t *cmdr)
{
    assert(ctx != NULL);
    assert(cmdr != NULL);
    assert(ctx->cmdp.priv != NULL);

    if (cmdr->res_id < 0)
        return;

    priv_t *priv = ctx->cmdp.priv;

    assert(priv->cmdp.entries[cmdr->res_id].in_use);

    priv->cmdp.entries[cmdr->res_id].in_use = 0;
    priv->cmdp.last_i = cmdr->res_id;
    priv->cmdp.cur_n--;

    if (priv->prio.res_id == cmdr->res_id) {
        priv->prio.pool_res = NULL;
        priv->prio.res_id = -1;
    }
}

void yf_cmdpool_reset(yf_context_t *ctx, yf_cmdres_t *cmdr)
{
    assert(ctx != NULL);
    assert(cmdr != NULL);
    assert(ctx->cmdp.priv != NULL);

    if (cmdr->res_id < 0)
        return;

    priv_t *priv = ctx->cmdp.priv;
    /* XXX: This assumes that every resource has an exclusive pool. */
    vkResetCommandPool(ctx->device, priv->cmdp.entries[cmdr->res_id].pool,
                       VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

    yf_cmdpool_yield(ctx, cmdr);
}

const yf_cmdres_t *yf_cmdpool_getprio(yf_context_t *ctx,
                                      void (*callb)(int res, void *arg),
                                      void *arg)
{
    assert(ctx != NULL);
    assert(ctx->cmdp.priv != NULL);

    priv_t *priv = ctx->cmdp.priv;

    if (priv->prio.res_id == -1) {
        if (yf_cmdpool_obtain(ctx, &priv->prio) != 0)
            return NULL;
        VkCommandBufferBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = NULL
        };
        if (vkBeginCommandBuffer(priv->prio.pool_res, &info) != VK_SUCCESS) {
            yf_seterr(YF_ERR_DEVGEN, __func__);
            yf_cmdpool_yield(ctx, &priv->prio);
            return NULL;
        }
    }

    if (callb != NULL) {
        callb_t *e = malloc(sizeof(callb_t));
        if (e == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return NULL;
        }
        e->callb = callb;
        e->arg = arg;
        if (yf_list_insert(priv->callbs, e) != 0) {
            free(e);
            return NULL;
        }
    }

    return &priv->prio;
}

void yf_cmdpool_checkprio(yf_context_t *ctx, const yf_cmdres_t **cmdr_list,
                          unsigned *cmdr_n)
{
    assert(ctx != NULL);
    assert(cmdr_list != NULL);
    assert(cmdr_n != NULL);
    assert(ctx->cmdp.priv != NULL);

    priv_t *priv = ctx->cmdp.priv;
    if (priv->prio.res_id == -1) {
        *cmdr_list = NULL;
        *cmdr_n = 0;
    } else {
        *cmdr_list = &priv->prio;
        *cmdr_n = 1;
    }
}

void yf_cmdpool_notifyprio(yf_context_t *ctx, int result)
{
    assert(ctx != NULL);
    assert(ctx->cmdp.priv != NULL);

    priv_t *priv = ctx->cmdp.priv;
    yf_cmdpool_yield(ctx, &priv->prio);

    if (yf_list_getlen(priv->callbs) < 1)
        return;

    yf_iter_t it = YF_NILIT;
    callb_t *callb = NULL;
    while (1) {
        callb = yf_list_next(priv->callbs, &it);
        if (YF_IT_ISNIL(it))
            break;
        callb->callb(result, callb->arg);
        free(callb);
    }
    yf_list_clear(priv->callbs);
}
