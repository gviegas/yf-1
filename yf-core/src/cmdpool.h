/*
 * YF
 * cmdpool.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_CMDPOOL_H
#define YF_CMDPOOL_H

#include "yf-context.h"
#include "vk.h"

/* Type defining a resource acquired from the command pool. */
typedef struct {
  VkCommandBuffer pool_res;
  int res_id;
} YF_cmdres;

/* Creates a new command pool. */
int yf_cmdpool_create(YF_context ctx, unsigned capacity);

/* Obtains a resource for a given command buffer type. */
int yf_cmdpool_obtain(YF_context ctx, int cmdbuf, YF_cmdres *cmdr);

/* Yields a previously obtained resource. */
void yf_cmdpool_yield(YF_context ctx, YF_cmdres *cmdr);

/* Resets and yields a resource. */
void yf_cmdpool_reset(YF_context ctx, YF_cmdres *cmdr);

/* Gets the priority command pool resource for a given context.
   If there is no resource in use, one supporting the given 'cmdbuf' value
   is created and put in the recording state. */
const YF_cmdres *yf_cmdpool_getprio(YF_context ctx, int cmdbuf,
    void (*callb)(int res, void *arg), void *arg);

/* Checks which priority resources have been used and are pending execution. */
void yf_cmdpool_checkprio(YF_context ctx, const YF_cmdres **cmdr_list,
    unsigned *cmdr_n);

/* Notifies that a pending priority resource did execute. */
void yf_cmdpool_notifyprio(YF_context ctx, int result);

#endif /* YF_CMDPOOL_H */
