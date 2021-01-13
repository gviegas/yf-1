/*
 * YF
 * cmdexec.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <yf/com/yf-util.h>
#include <yf/com/yf-list.h>
#include <yf/com/yf-error.h>

#include "cmdexec.h"
#include "context.h"
#include "cmdbuf.h"

#define YF_CMDEMIN 2
#define YF_CMDEMAX 32

#define YF_CMDEWAIT 16666666UL

/* Type defining an entry in the queue. */
typedef struct {
  YF_cmdres cmdr;
  void (*callb)(int, void *);
  void *arg;
} L_entry;

/* Type defining queue variables. */
typedef struct {
  L_entry *entries;
  VkCommandBuffer *buffers;
  unsigned n;
  VkSubmitInfo subm_info;
  VkQueue queue;
  VkFence fence;
} L_qvars;

/* Type defining a command execution queue. */
typedef struct {
  int q1_i;
  int q2_i;
  L_qvars *q1;
  L_qvars *q2;
  unsigned cap;
} L_cmde;

/* Type defining execution queues stored in a context. */
typedef struct {
  L_cmde *cmde;
  L_cmde *prio;
  YF_list fences;
} L_priv;

/* Initializes a pre-allocated queue. */
static int init_queue(YF_context ctx, L_cmde *cmde);

/* Enqueues commands in a queue. */
static int enqueue_res(L_cmde *cmde, const YF_cmdres *cmdr,
    void (*callb)(int res, void *arg), void *arg);

/* Executes a command queue. */
static int exec_queue(YF_context ctx, L_cmde *cmde);

/* Resets a command queue. */
static void reset_queue(YF_context ctx, L_cmde *cmde);

/* Deinitializes and deallocates a queue. */
static void deinit_queue(YF_context ctx, L_cmde *cmde);

/* Destroys the 'L_priv' data stored in a given context. */
static void destroy_priv(YF_context ctx);

int yf_cmdexec_create(YF_context ctx, unsigned capacity) {
  assert(ctx != NULL);

  if (ctx->cmde.priv != NULL)
    destroy_priv(ctx);

  L_priv *priv = calloc(1, sizeof(L_priv));
  if (priv == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  priv->cmde = calloc(1, sizeof(L_cmde));
  priv->prio = calloc(1, sizeof(L_cmde));
  priv->fences = yf_list_init(NULL);
  if (priv->cmde == NULL || priv->prio == NULL || priv->fences == NULL) {
    if (priv->fences != NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      yf_list_deinit(priv->fences);
    }
    free(priv->cmde);
    free(priv->prio);
    free(priv);
    return -1;
  }

  ctx->cmde.priv = priv;
  ctx->cmde.deinit_callb = destroy_priv;

  priv->cmde->cap = YF_MAX(YF_CMDEMIN, YF_MIN(YF_CMDEMAX, capacity));
  if (init_queue(ctx, priv->cmde) != 0) {
    priv->cmde = NULL;
    destroy_priv(ctx);
    return -1;
  }
  priv->prio->cap = YF_CMDEMIN;
  if (init_queue(ctx, priv->prio) != 0) {
    priv->prio = NULL;
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

  return enqueue_res(((L_priv *)ctx->cmde.priv)->cmde, cmdr, callb, arg);
}

int yf_cmdexec_exec(YF_context ctx) {
  assert(ctx != NULL);

  if (yf_cmdexec_execprio(ctx) != 0) {
    reset_queue(ctx, ((L_priv *)ctx->cmde.priv)->cmde);
    return -1;
  }
  return exec_queue(ctx, ((L_priv *)ctx->cmde.priv)->cmde);
}

int yf_cmdexec_execprio(YF_context ctx) {
  assert(ctx != NULL);
  assert(ctx->cmde.priv != NULL);

  L_priv *priv = ctx->cmde.priv;
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

void yf_cmdexec_reset(YF_context ctx) {
  assert(ctx != NULL);
  assert(ctx->cmde.priv != NULL);

  reset_queue(ctx, ((L_priv *)ctx->cmde.priv)->cmde);
}

void yf_cmdexec_resetprio(YF_context ctx) {
  assert(ctx != NULL);
  assert(ctx->cmde.priv != NULL);

  reset_queue(ctx, ((L_priv *)ctx->cmde.priv)->prio);
}

void yf_cmdexec_waitfor(YF_context ctx, VkFence fence) {
  assert(ctx != NULL);
  assert(ctx->cmde.priv != NULL);

  YF_list fences = ((L_priv *)ctx->cmde.priv)->fences;
  yf_list_insert(fences, fence);
}

static int init_queue(YF_context ctx, L_cmde *cmde) {
  assert(ctx != NULL);
  assert(cmde != NULL);
  assert(cmde->cap > 0);

  cmde->q2_i = -1;
  cmde->q2 = NULL;
  unsigned queue_n = 0;
  if (ctx->graph_queue_i >= 0) {
    cmde->q1_i = ctx->graph_queue_i;
    ++queue_n;
  }
  if (ctx->comp_queue_i != ctx->graph_queue_i) {
    if (queue_n == 0)
      cmde->q1_i = ctx->comp_queue_i;
    else
      cmde->q2_i = ctx->comp_queue_i;
    ++queue_n;
  }

  L_qvars **qvs[2] = {&cmde->q1, &cmde->q2};
  for (unsigned i = 0; i < queue_n; ++i) {
    L_qvars *qv = malloc(sizeof(L_qvars));
    if (qv == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      deinit_queue(ctx, cmde);
      return -1;
    }
    qv->entries = malloc(sizeof(L_entry) * cmde->cap);
    if (qv->entries == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      deinit_queue(ctx, cmde);
      return -1;
    }
    qv->buffers = malloc(sizeof(VkCommandBuffer) * cmde->cap);
    if (qv->buffers == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      deinit_queue(ctx, cmde);
      return -1;
    }
    VkFenceCreateInfo fence_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0
    };
    VkResult res = vkCreateFence(ctx->device, &fence_info, NULL, &qv->fence);
    if (res != VK_SUCCESS) {
      yf_seterr(YF_ERR_DEVGEN, __func__);
      deinit_queue(ctx, cmde);
      return -1;
    }
    qv->subm_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    qv->subm_info.pNext = NULL;
    qv->subm_info.waitSemaphoreCount = 0;
    qv->subm_info.pWaitSemaphores = 0;
    qv->subm_info.pWaitDstStageMask = NULL;
    qv->subm_info.commandBufferCount = 0;
    qv->subm_info.pCommandBuffers = qv->buffers;
    qv->subm_info.signalSemaphoreCount = 0;
    qv->subm_info.pSignalSemaphores = NULL;
    if (cmde->q1_i == ctx->graph_queue_i)
      qv->queue = ctx->graph_queue;
    else
      qv->queue = ctx->comp_queue;
    qv->n = 0;
    *qvs[i] = qv;
  }

  return 0;
}

static int enqueue_res(L_cmde *cmde, const YF_cmdres *cmdr,
    void (*callb)(int res, void *arg), void *arg)
{
  assert(cmde != NULL);
  assert(cmdr != NULL);

  L_qvars *qv = NULL;
  if (cmdr->queue_i == cmde->q1_i)
    qv = cmde->q1;
  else if (cmdr->queue_i == cmde->q2_i)
    qv = cmde->q2;
  else
    assert(0);
  if (qv->n == cmde->cap) {
    yf_seterr(YF_ERR_QFULL, __func__);
    return -1;
  }
  memcpy(&qv->entries[qv->n].cmdr, cmdr, sizeof *cmdr);
  qv->entries[qv->n].callb = callb;
  qv->entries[qv->n].arg = arg;
  qv->buffers[qv->n] = cmdr->pool_res;
  ++qv->n;

  return 0;
}

static int exec_queue(YF_context ctx, L_cmde *cmde) {
  assert(ctx != NULL);
  assert(cmde != NULL);

  /* TODO: Use signal/wait semaphores between priority and non-priority
     command buffers instead of multiple submissions. */

  int r = 0;
  L_qvars *qvs[2] = {cmde->q1, cmde->q2};
  VkFence fences[2] = {NULL, NULL};
  unsigned fence_n = 0;
  VkResult res;

  for (unsigned i = 0; i < 2; ++i) {
    if (qvs[i] == NULL || qvs[i]->n < 1)
      continue;
    qvs[i]->subm_info.commandBufferCount = qvs[i]->n;
    res = vkQueueSubmit(qvs[i]->queue, 1, &qvs[i]->subm_info, qvs[i]->fence);
    if (res != VK_SUCCESS) {
      yf_seterr(YF_ERR_DEVGEN, __func__);
      r = -1;
    } else {
      fences[fence_n++] = qvs[i]->fence;
    }
  }

  if (fence_n > 0) {
    do
      res = vkWaitForFences(ctx->device, fence_n, fences, VK_TRUE, YF_CMDEWAIT);
    while (res == VK_TIMEOUT);
    if (res != VK_SUCCESS) {
      yf_seterr(YF_ERR_DEVGEN, __func__);
      r = -1;
    }
  }

  for (unsigned i = 0; i < 2; ++i) {
    if (qvs[i] == NULL || qvs[i]->n < 1)
      continue;
    for (unsigned j = 0; j < qvs[i]->n; ++j) {
      yf_cmdpool_yield(ctx, &qvs[i]->entries[j].cmdr);
      if (qvs[i]->entries[j].callb != NULL)
        qvs[i]->entries[j].callb(r, qvs[i]->entries[j].arg);
    }
    qvs[i]->n = 0;
  }

  return r;
}

static void reset_queue(YF_context ctx, L_cmde *cmde) {
  assert(ctx != NULL);
  assert(cmde != NULL);

  L_qvars *qvs[2] = {cmde->q1, cmde->q2};
  for (unsigned i = 0; i < 2; ++i) {
    if (qvs[i] == NULL || qvs[i]->n < 1)
      continue;
    for (unsigned j = 0; j < qvs[i]->n; ++j) {
      yf_cmdpool_reset(ctx, &qvs[i]->entries[j].cmdr);
      if (qvs[i]->entries[j].callb != NULL)
        qvs[i]->entries[j].callb(-1, qvs[i]->entries[j].arg);
    }
    qvs[i]->n = 0;
  }
}

static void deinit_queue(YF_context ctx, L_cmde *cmde) {
  assert(ctx != NULL);

  if (cmde == NULL)
    return;

  reset_queue(ctx, cmde);
  L_qvars *qvs[2] = {cmde->q1, cmde->q2};
  for (unsigned i = 0; i < 2; ++i) {
    if (qvs[i] != NULL) {
      vkDestroyFence(ctx->device, qvs[i]->fence, NULL);
      free(qvs[i]->buffers);
      free(qvs[i]->entries);
      free(qvs[i]);
    }
  }
  free(cmde);
}

static void destroy_priv(YF_context ctx) {
  assert(ctx != NULL);

  if (ctx->cmde.priv == NULL)
    return;

  L_priv *priv = ctx->cmde.priv;
  deinit_queue(ctx, priv->cmde);
  deinit_queue(ctx, priv->prio);
  yf_list_deinit(priv->fences);
  free(priv);
  ctx->cmde.priv = NULL;
}
