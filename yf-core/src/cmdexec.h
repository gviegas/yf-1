/*
 * YF
 * cmdexec.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_CMDEXEC_H
#define YF_CMDEXEC_H

#include "cmdpool.h"

/* Creates a new command execution queue. */
int yf_cmdexec_create(YF_context ctx, unsigned capacity);

/* Enqueues a command pool resource for execution. */
int yf_cmdexec_enqueue(YF_context ctx, const YF_cmdres *cmdr,
    void (*callb)(int res, void *arg), void *arg);

/* Executes all commands currently in the queue. */
int yf_cmdexec_exec(YF_context ctx);

/* Executes priority commands only. */
int yf_cmdexec_execprio(YF_context ctx);

/* Discards pending commands and yield resources. */
void yf_cmdexec_reset(YF_context ctx);

/* Discards pending priority commands and yield resources. */
void yf_cmdexec_resetprio(YF_context ctx);

#endif /* YF_CMDEXEC_H */
