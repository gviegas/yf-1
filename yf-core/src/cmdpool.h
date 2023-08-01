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

/* Resource acquired from the command pool. */
typedef struct yf_cmdres {
    VkCommandBuffer pool_res;
    int res_id;
} yf_cmdres_t;

/* Creates a new command pool. */
int yf_cmdpool_create(yf_context_t *ctx, unsigned capacity);

/* Obtains a resource from the command pool. */
int yf_cmdpool_obtain(yf_context_t *ctx, yf_cmdres_t *cmdr);

/* Yields a previously obtained resource. */
void yf_cmdpool_yield(yf_context_t *ctx, yf_cmdres_t *cmdr);

/* Resets and yields a resource. */
void yf_cmdpool_reset(yf_context_t *ctx, yf_cmdres_t *cmdr);

/* Gets the priority command pool resource for a given context.
   If there is no resource in use, one is created and started. */
const yf_cmdres_t *yf_cmdpool_getprio(yf_context_t *ctx,
                                      void (*callb)(int res, void *arg),
                                      void *arg);

/* Checks which priority resources have been used and are pending execution. */
void yf_cmdpool_checkprio(yf_context_t *ctx, const yf_cmdres_t **cmdr_list,
                          unsigned *cmdr_n);

/* Notifies that a pending priority resource did execute. */
void yf_cmdpool_notifyprio(yf_context_t *ctx, int result);

#endif /* YF_CMDPOOL_H */
