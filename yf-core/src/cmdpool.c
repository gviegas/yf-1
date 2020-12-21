/*
 * YF
 * cmdpool.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include <yf/com/yf-list.h>
#include <yf/com/yf-error.h>

#include "cmdpool.h"
#include "cmdbuf.h"
#include "context.h"

#ifndef YF_MIN
# define YF_MIN(a, b) (a < b ? a : b)
#endif

#ifndef YF_MAX
# define YF_MAX(a, b) (a > b ? a : b)
#endif

#define YF_CMDPMIN 2
#define YF_CMDPMAX 32

/* Type representing a resource entry in the pool. */
typedef struct {
  VkCommandPool pool;
  VkCommandBuffer buffer;
  int queue_i;
  int in_use;
} L_entry;

/* Type defining a command pool. */
typedef struct {
  L_entry *entries;
  unsigned last_i;
  unsigned curr_n;
  unsigned cap;
} L_cmdp;

/* Type defining command pool variables stored in a context. */
typedef struct {
  L_cmdp *cmdp;
  YF_cmdpres prio[2];
  unsigned prio_n;
  YF_list callbs;
} L_priv;

/* Type storing a callback from priority resource acquisition. */
typedef struct {
  void (*callb)(int, void *);
  void *data;
} L_callb;

/* Initializes the pool entries. */
static int init_entries(YF_context ctx, L_cmdp *cmdp);

/* Destroys the 'L_priv' data stored in a given context. */
static void destroy_priv(YF_context ctx);

int yf_cmdpool_create(YF_context ctx, unsigned capacity) {
  assert(ctx != NULL);

  if (ctx->cmdp.priv != NULL)
    destroy_priv(ctx);

  L_priv *priv = calloc(1, sizeof(L_priv));
  if (priv == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  priv->cmdp = calloc(1, sizeof(L_cmdp));
  if (priv->cmdp == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(priv);
    return -1;
  }

  ctx->cmdp.priv= priv;
  ctx->cmdp.deinit_callb = destroy_priv;

  priv->cmdp->last_i = 0;
  priv->cmdp->curr_n = 0;
  priv->cmdp->cap = YF_MAX(YF_CMDPMIN, YF_MIN(YF_CMDPMAX, capacity));
  if (init_entries(ctx, priv->cmdp) != 0) {
    destroy_priv(ctx);
    return  -1;
  }
  for (unsigned i = 0; i < (sizeof priv->prio / sizeof priv->prio[0]); ++i) {
    priv->prio[i].pool_res = NULL;
    priv->prio[i].res_id = -1;
    priv->prio[i].queue_i = -1;
  }
  priv->prio_n = 0;
  priv->callbs = yf_list_init(NULL);
  if (priv->callbs == NULL) {
    destroy_priv(ctx);
    return -1;
  }

  return 0;
}

int yf_cmdpool_obtain(YF_context ctx, int cmdbuf, YF_cmdpres *pres) {
  assert(ctx != NULL);
  assert(pres != NULL);
  assert(ctx->cmdp.priv != NULL);

  L_cmdp *cmdp = ((L_priv *)ctx->cmdp.priv)->cmdp;
  if (cmdp->curr_n == cmdp->cap) {
    yf_seterr(YF_ERR_INUSE, __func__);
    return -1;
  }

  int queue_i = -1;
  switch (cmdbuf) {
    case YF_CMDBUF_GRAPH:
      queue_i = ctx->graph_queue_i;
      break;
    case YF_CMDBUF_COMP:
      queue_i = ctx->comp_queue_i;
      break;
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      return -1;
  }
  if (queue_i == -1) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    return -1;
  }

  L_entry *e = NULL;
  for (unsigned i = 0; i < cmdp->cap; ++i) {
    e = &cmdp->entries[cmdp->last_i];
    if (!e->in_use && e->queue_i == queue_i) {
      pres->queue_i = queue_i;
      pres->res_id = cmdp->last_i;
      pres->pool_res = e->buffer;
      e->in_use = 1;
      ++cmdp->curr_n;
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

void yf_cmdpool_yield(YF_context ctx, YF_cmdpres *pres) {
  assert(ctx != NULL);
  assert(pres != NULL);
  assert(ctx->cmdp.priv != NULL);

  if (pres->res_id < 0)
    return;

  L_priv *priv = ctx->cmdp.priv;
  assert(priv->cmdp->entries[pres->res_id].in_use);

  priv->cmdp->entries[pres->res_id].in_use = 0;
  priv->cmdp->last_i = pres->res_id;
  --priv->cmdp->curr_n;
  for (unsigned i = 0; i < priv->prio_n; ++i) {
    if (priv->prio[i].res_id == pres->res_id) {
      priv->prio[i].pool_res = NULL;
      priv->prio[i].res_id = -1;
      priv->prio[i].queue_i = -1;
      --priv->prio_n;
      break;
    }
  }
}

void yf_cmdpool_reset(YF_context ctx, YF_cmdpres *pres) {
  assert(ctx != NULL);
  assert(pres != NULL);
  assert(ctx->cmdp.priv != NULL);

  if (pres->res_id < 0)
    return;

  L_priv *priv = ctx->cmdp.priv;
  /* XXX: This assumes that every resource has an exclusive pool. */
  vkResetCommandPool(ctx->device, priv->cmdp->entries[pres->res_id].pool,
      VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

  yf_cmdpool_yield(ctx, pres);
}

const YF_cmdpres *yf_cmdpool_getprio(YF_context ctx, int cmdbuf,
    void (*callb)(int res, void *data), void *data)
{
  assert(ctx != NULL);
  assert(ctx->cmdp.priv != NULL);

  L_priv *priv = ctx->cmdp.priv;
  int queue_i = -1;
  switch (cmdbuf) {
    case YF_CMDBUF_GRAPH:
      queue_i = ctx->graph_queue_i;
      break;
    case YF_CMDBUF_COMP:
      queue_i = ctx->comp_queue_i;
      break;
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      return NULL;
  }
  if (queue_i == -1) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    return NULL;
  }

  YF_cmdpres *pres = NULL;
  for (unsigned i = 0; i < priv->prio_n; ++i) {
    if (priv->prio[i].queue_i == queue_i) {
      pres = priv->prio+i;
      break;
    }
  }

  if (pres == NULL) {
    pres = priv->prio+priv->prio_n;
    if (yf_cmdpool_obtain(ctx, cmdbuf, pres) != 0)
      return NULL;
    ++priv->prio_n;
    VkCommandBufferBeginInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = NULL,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = NULL
    };
    if (vkBeginCommandBuffer(pres->pool_res, &info) != VK_SUCCESS) {
      yf_seterr(YF_ERR_DEVGEN, __func__);
      yf_cmdpool_yield(ctx, pres);
      return NULL;
    }
  }

  if (callb != NULL) {
    L_callb *e = malloc(sizeof(L_callb));
    if (e == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return NULL;
    }
    e->callb= callb;
    e->data = data;
    if (yf_list_insert(priv->callbs, e) != 0)
      return NULL;
  }

  return pres;
}

void yf_cmdpool_checkprio(YF_context ctx, const YF_cmdpres **pres_list,
    unsigned *pres_n)
{
  assert(ctx != NULL);
  assert(pres_list != NULL);
  assert(pres_n != NULL);
  assert(ctx->cmdp.priv != NULL);

  L_priv *priv = ctx->cmdp.priv;
  if (priv->prio_n == 0) {
    *pres_list = NULL;
    *pres_n = 0;
  } else {
    *pres_list = priv->prio;
    *pres_n = priv->prio_n;
  }
}

void yf_cmdpool_notifyprio(YF_context ctx, int result) {
  assert(ctx != NULL);
  assert(ctx->cmdp.priv != NULL);

  L_priv *priv = ctx->cmdp.priv;
  for (unsigned i = 0; i < priv->prio_n; ++i)
    yf_cmdpool_yield(ctx, priv->prio+i);

  if (yf_list_getlen(priv->callbs) < 1)
    return;

  YF_iter it = YF_NILIT;
  L_callb *callb = NULL;
  do {
    callb = yf_list_next(priv->callbs, &it);
    if (YF_IT_ISNIL(it))
      break;
    callb->callb(result, callb->data);
    free(callb);
  } while (1);
  yf_list_clear(priv->callbs);
}

static int init_entries(YF_context ctx, L_cmdp *cmdp) {
  assert(ctx != NULL);
  assert(cmdp != NULL);

  cmdp->entries = calloc(cmdp->cap, sizeof(L_entry));
  if (cmdp->entries == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }

  const int gqueue_i = ctx->graph_queue_i;
  const int cqueue_i = ctx->comp_queue_i;
  VkCommandPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = 0
  };
  VkCommandBufferAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = NULL,
    .commandPool = NULL,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1
  };
  VkResult res;

  for (unsigned i = 0; i < cmdp->cap; ++i) {
    if (gqueue_i < 0)
      cmdp->entries[i].queue_i = cqueue_i;
    else if (cqueue_i < 0 || i % 2 == 0)
      cmdp->entries[i].queue_i = gqueue_i;
    else
      cmdp->entries[i].queue_i = cqueue_i;
    pool_info.queueFamilyIndex = cmdp->entries[i].queue_i;

    /* TODO: Consider using only one pool for each queue instead. */
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

static void destroy_priv(YF_context ctx) {
  assert(ctx != NULL);

  if (ctx->cmdp.priv == NULL)
    return;

  L_priv *priv = ctx->cmdp.priv;
  YF_iter it = YF_NILIT;
  do
    free(yf_list_next(priv->callbs, &it));
  while (!YF_IT_ISNIL(it));
  yf_list_deinit(priv->callbs);

  if (priv->cmdp->entries != NULL) {
    for (unsigned i = 0; i < priv->cmdp->cap; ++i)
      vkDestroyCommandPool(ctx->device, priv->cmdp->entries[i].pool, NULL);
    free(priv->cmdp->entries);
  }
  free(priv->cmdp);

  free(priv);
  ctx->cmdp.priv = NULL;
}
