/*
 * YF
 * cmdexec.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_CMDEXEC_H
#define YF_CMDEXEC_H

#include "yf-context.h"
#include "vk.h"
#include "cmdpool.h"

/* Creates a new command execution queue. */
int yf_cmdexec_create(yf_context_t *ctx, unsigned capacity);

/* Enqueues a command pool resource for execution. */
int yf_cmdexec_enqueue(yf_context_t *ctx, const yf_cmdres_t *cmdr,
                       void (*callb)(int res, void *arg), void *arg);

/* Executes all commands currently in the queue. */
int yf_cmdexec_exec(yf_context_t *ctx);

/* Executes priority commands only. */
int yf_cmdexec_execprio(yf_context_t *ctx);

/* Discards pending commands and yield resources. */
void yf_cmdexec_reset(yf_context_t *ctx);

/* Discards pending priority commands and yield resources. */
void yf_cmdexec_resetprio(yf_context_t *ctx);

/* Sets a semaphore upon which to wait in the next submission. */
void yf_cmdexec_waitfor(yf_context_t *ctx, VkSemaphore sem,
                        VkPipelineStageFlags stg_mask);

#endif /* YF_CMDEXEC_H */
