/*
 * YF
 * cmdpool.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
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

/* Type representing a resource entry in the pool. */
typedef struct {
    VkCommandPool pool;
    VkCommandBuffer buffer;
    int in_use;
} T_entry;

/* Type defining a command pool. */
typedef struct {
    T_entry *entries;
    unsigned last_i;
    unsigned cur_n;
    unsigned cap;
} T_cmdp;

/* Type defining command pool variables stored in a context. */
typedef struct {
    T_cmdp cmdp;
    YF_cmdres prio;
    YF_list callbs;
} T_priv;

/* Type storing a callback from priority resource acquisition. */
typedef struct {
    void (*callb)(int, void *);
    void *arg;
} T_callb;

/* Initializes the pool entries. */
static int init_entries(YF_context ctx, T_cmdp *cmdp)
{
    assert(ctx != NULL);
    assert(cmdp != NULL);

    cmdp->entries = calloc(cmdp->cap, sizeof(T_entry));
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

/* Destroys the 'T_priv' data stored in a given context. */
static void destroy_priv(YF_context ctx)
{
    assert(ctx != NULL);

    if (ctx->cmdp.priv == NULL)
        return;

    T_priv *priv = ctx->cmdp.priv;
    YF_iter it = YF_NILIT;
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

int yf_cmdpool_create(YF_context ctx, unsigned capacity)
{
    assert(ctx != NULL);

    if (ctx->cmdp.priv != NULL)
        destroy_priv(ctx);

    T_priv *priv = calloc(1, sizeof(T_priv));
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

int yf_cmdpool_obtain(YF_context ctx, YF_cmdres *cmdr)
{
    assert(ctx != NULL);
    assert(cmdr != NULL);
    assert(ctx->cmdp.priv != NULL);

    T_cmdp *cmdp = &((T_priv *)ctx->cmdp.priv)->cmdp;
    if (cmdp->cur_n == cmdp->cap) {
        yf_seterr(YF_ERR_INUSE, __func__);
        return -1;
    }

    T_entry *e = NULL;
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

void yf_cmdpool_yield(YF_context ctx, YF_cmdres *cmdr)
{
    assert(ctx != NULL);
    assert(cmdr != NULL);
    assert(ctx->cmdp.priv != NULL);

    if (cmdr->res_id < 0)
        return;

    T_priv *priv = ctx->cmdp.priv;

    assert(priv->cmdp.entries[cmdr->res_id].in_use);

    priv->cmdp.entries[cmdr->res_id].in_use = 0;
    priv->cmdp.last_i = cmdr->res_id;
    priv->cmdp.cur_n--;

    if (priv->prio.res_id == cmdr->res_id) {
        priv->prio.pool_res = NULL;
        priv->prio.res_id = -1;
    }
}

void yf_cmdpool_reset(YF_context ctx, YF_cmdres *cmdr)
{
    assert(ctx != NULL);
    assert(cmdr != NULL);
    assert(ctx->cmdp.priv != NULL);

    if (cmdr->res_id < 0)
        return;

    T_priv *priv = ctx->cmdp.priv;
    /* XXX: This assumes that every resource has an exclusive pool. */
    vkResetCommandPool(ctx->device, priv->cmdp.entries[cmdr->res_id].pool,
                       VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

    yf_cmdpool_yield(ctx, cmdr);
}

const YF_cmdres *yf_cmdpool_getprio(YF_context ctx,
                                    void (*callb)(int res, void *arg),
                                    void *arg)
{
    assert(ctx != NULL);
    assert(ctx->cmdp.priv != NULL);

    T_priv *priv = ctx->cmdp.priv;

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
        T_callb *e = malloc(sizeof(T_callb));
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

void yf_cmdpool_checkprio(YF_context ctx, const YF_cmdres **cmdr_list,
                          unsigned *cmdr_n)
{
    assert(ctx != NULL);
    assert(cmdr_list != NULL);
    assert(cmdr_n != NULL);
    assert(ctx->cmdp.priv != NULL);

    T_priv *priv = ctx->cmdp.priv;
    if (priv->prio.res_id == -1) {
        *cmdr_list = NULL;
        *cmdr_n = 0;
    } else {
        *cmdr_list = &priv->prio;
        *cmdr_n = 1;
    }
}

void yf_cmdpool_notifyprio(YF_context ctx, int result)
{
    assert(ctx != NULL);
    assert(ctx->cmdp.priv != NULL);

    T_priv *priv = ctx->cmdp.priv;
    yf_cmdpool_yield(ctx, &priv->prio);

    if (yf_list_getlen(priv->callbs) < 1)
        return;

    YF_iter it = YF_NILIT;
    T_callb *callb = NULL;
    while (1) {
        callb = yf_list_next(priv->callbs, &it);
        if (YF_IT_ISNIL(it))
            break;
        callb->callb(result, callb->arg);
        free(callb);
    }
    yf_list_clear(priv->callbs);
}
