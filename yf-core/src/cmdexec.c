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

#define YF_CMDEMIN 1
#define YF_CMDEMAX 32

#define YF_CMDEWAIT 16666666UL

/* Type defining an entry in the queue. */
typedef struct {
  YF_cmdres cmdr;
  void (*callb)(int, void *);
  void *arg;
} T_entry;

/* Type defining a command execution queue. */
typedef struct {
  T_entry *entries;
  VkCommandBuffer *buffers;
  unsigned n;
  unsigned cap;
  VkSubmitInfo subm_info;
  VkQueue queue;
  VkFence fence;
} T_cmde;

/* Type defining execution queues stored in a context. */
typedef struct {
  T_cmde cmde;
  T_cmde prio;
  YF_list fences;
} T_priv;

/* Initializes a pre-allocated queue. */
static int init_queue(YF_context ctx, T_cmde *cmde);

/* Enqueues commands in a queue. */
static int enqueue_res(T_cmde *cmde, const YF_cmdres *cmdr,
    void (*callb)(int res, void *arg), void *arg);

/* Executes a command queue. */
static int exec_queue(YF_context ctx, T_cmde *cmde);

/* Resets a command queue. */
static void reset_queue(YF_context ctx, T_cmde *cmde);

/* Deinitializes and deallocates a queue. */
static void deinit_queue(YF_context ctx, T_cmde *cmde);

/* Destroys the 'T_priv' data stored in a given context. */
static void destroy_priv(YF_context ctx);

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
  priv->fences = yf_list_init(NULL);
  if (priv->fences == NULL) {
    free(priv);
    return -1;
  }

  ctx->cmde.priv = priv;
  ctx->cmde.deinit_callb = destroy_priv;

  priv->cmde.cap = YF_CLAMP(capacity, YF_CMDEMIN, YF_CMDEMAX);
  if (init_queue(ctx, &priv->cmde) != 0) {
    destroy_priv(ctx);
    return -1;
  }
  priv->prio.cap = YF_CMDEMIN;
  if (init_queue(ctx, &priv->prio) != 0) {
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

  return enqueue_res(((T_priv *)ctx->cmde.priv)->cmde, cmdr, callb, arg);
}

int yf_cmdexec_exec(YF_context ctx)
{
  assert(ctx != NULL);

  if (yf_cmdexec_execprio(ctx) != 0) {
    reset_queue(ctx, ((T_priv *)ctx->cmde.priv)->cmde);
    return -1;
  }
  return exec_queue(ctx, ((T_priv *)ctx->cmde.priv)->cmde);
}

int yf_cmdexec_execprio(YF_context ctx)
{
  assert(ctx != NULL);
  assert(ctx->cmde.priv != NULL);

  T_priv *priv = ctx->cmde.priv;
  int r = 0;

  const size_t fence_n = yf_list_getlen(priv->fences);
  if (fence_n > 0) {
    VkFence fences[fence_n];
    VkResult res;
    for (unsigned i = 0; i < fence_n; ++i)
      fences[i] = yf_list_removeat(priv->fences, NULL);
    do
      res = vkWaitForFences(ctx->device, fence_n, fences, VK_TRUE, YF_CMDEWAIT);
    while (res == VK_TIMEOUT);
    if (res != VK_SUCCESS)
      r = -1;
  }

  const YF_cmdres *cmdr_list;
  unsigned cmdr_n;
  yf_cmdpool_checkprio(ctx, &cmdr_list, &cmdr_n);

  if (cmdr_n == 0)
    return r;

  for (unsigned i = 0; i < cmdr_n; ++i) {
    if (vkEndCommandBuffer(cmdr_list[i].pool_res) != VK_SUCCESS) {
      yf_seterr(YF_ERR_DEVGEN, __func__);
      r = -1;
      break;
    }
    if (enqueue_res(priv->prio, cmdr_list+i, NULL, NULL) != 0) {
      r = -1;
      break;
    }
  }

  if (r == 0)
    r = exec_queue(ctx, priv->prio);
  else
    reset_queue(ctx, priv->prio);

  yf_cmdpool_notifyprio(ctx, r);
  return r;
}

void yf_cmdexec_reset(YF_context ctx)
{
  assert(ctx != NULL);
  assert(ctx->cmde.priv != NULL);

  reset_queue(ctx, ((T_priv *)ctx->cmde.priv)->cmde);
}

void yf_cmdexec_resetprio(YF_context ctx)
{
  assert(ctx != NULL);
  assert(ctx->cmde.priv != NULL);

  reset_queue(ctx, ((T_priv *)ctx->cmde.priv)->prio);
}

void yf_cmdexec_waitfor(YF_context ctx, VkFence fence)
{
  assert(ctx != NULL);
  assert(ctx->cmde.priv != NULL);

  YF_list fences = ((T_priv *)ctx->cmde.priv)->fences;
  yf_list_insert(fences, fence);
}

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

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0
  };
  VkResult res = vkCreateFence(ctx->device, &fence_info, NULL, &cmde->fence);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    deinit_queue(ctx, cmde);
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

  cmde->queue = ctx->queue;
  cmde->n = 0;
  return 0;
}

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
  ++cmde->n;
  return 0;
}

static int exec_queue(YF_context ctx, T_cmde *cmde)
{
  assert(ctx != NULL);
  assert(cmde != NULL);

  /* TODO: Use signal/wait semaphores between priority and non-priority
     command buffers instead of multiple submissions. */

  if (cmde->n < 1)
    return 0;

  int r = 0;
  VkResult res;

  cmde->subm_info.commandBufferCount = cmde->n;
  res = vkQueueSubmit(cmde->queue, 1, &cmde->subm_info, cmde->fence);

  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    r = -1;
  } else {
    do
      res = vkWaitForFences(ctx->device, 1, &cmde->fence, VK_TRUE, YF_CMDEWAIT);
    while (res == VK_TIMEOUT);
    if (res != VK_SUCCESS) {
      yf_seterr(YF_ERR_DEVGEN, __func__);
      r = -1;
    }
  }

  for (unsigned i = 0; i < cmde->n; ++i) {
    yf_cmdpool_yield(ctx, &cmde->entries[i].cmdr);
    if (cmde->entries[i].callb != NULL)
      cmde->entries[i].callb(r, cmde->entries[i].arg);
  }
  cmde->n = 0;
  return r;
}

static void reset_queue(YF_context ctx, T_cmde *cmde)
{
  assert(ctx != NULL);
  assert(cmde != NULL);

  if (cmde->n < 1)
    return;

  for (unsigned i = 0; i < cmde->n; ++i) {
    yf_cmdpool_reset(ctx, &cmde->entries[i].cmdr);
    if (cmde->entries[i].callb != NULL)
      cmde->entries[i].callb(-1, cmde->entries[i].arg);
  }
  cmde->n = 0;
}

static void deinit_queue(YF_context ctx, T_cmde *cmde)
{
  assert(ctx != NULL);

  if (cmde == NULL)
    return;

  reset_queue(ctx, cmde);
  vkDestroyFence(ctx->device, cmde->fence, NULL);
  free(cmde->buffers);
  free(cmde->entries);
  free(cmde);
}

static void destroy_priv(YF_context ctx)
{
  assert(ctx != NULL);

  if (ctx->cmde.priv == NULL)
    return;

  T_priv *priv = ctx->cmde.priv;
  deinit_queue(ctx, priv->cmde);
  deinit_queue(ctx, priv->prio);
  yf_list_deinit(priv->fences);
  free(priv);
  ctx->cmde.priv = NULL;
}
