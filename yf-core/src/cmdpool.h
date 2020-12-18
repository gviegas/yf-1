/*
 * YF
 * cmdpool.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_CMDPOOL_H
#define YF_CMDPOOL_H

#include "yf-context.h"
#include "vk.h"

/* Type defining a resource acquired from the command pool. */
typedef struct {
  VkCommandBuffer pool_res;
  int res_id;
  int queue_i;
} YF_cmdpres;

/* Creates a new command pool. */
int yf_cmdpool_create(YF_context ctx, unsigned capacity);

/* Obtains a resource for a given command buffer type. */
int yf_cmdpool_obtain(YF_context ctx, int cmdb, YF_cmdpres *pres);

/* Yields a previously obtained resource. */
void yf_cmdpool_yield(YF_context ctx, YF_cmdpres *pres);

/* Resets and yields a resource. */
void yf_cmdpool_reset(YF_context ctx, YF_cmdpres *pres);

/* Gets the priority command pool resource for a given context.
   If there is no resource in use, one supporting the given 'cmdb' value
   is created and put in the recording state. */
const YF_cmdpres *yf_cmdpool_getprio(YF_context ctx, int cmdb,
    void (*callb)(int res, void *data), void *data);

/* Checks which priority resources have been used and are pending execution. */
void yf_cmdpool_checkprio(YF_context ctx, const YF_cmdpres **pres_list,
    unsigned *pres_n);

/* Notifies that a pending priority resource did execute. */
void yf_cmdpool_notifyprio(YF_context ctx, int result);

#endif /* YF_CMDPOOL_H */
