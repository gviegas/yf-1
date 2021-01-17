/*
 * YF
 * debug.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#if defined(YF_DEBUG)
# include <stdio.h>
# include "debug.h"

void yf_debug_ctx(YF_context ctx) {
  printf("\n-- Context (debug) --");

  printf("\nqueue indices (graph/comp/pres): %d/%d/%d",
      ctx->graph_queue_i, ctx->comp_queue_i, ctx->pres_queue_i);
  printf("\ninstance version: %u.%u", VK_VERSION_MAJOR(ctx->inst_version),
      VK_VERSION_MINOR(ctx->inst_version));

  printf("\nlayers: #%u", ctx->layer_n);
  for (unsigned i = 0; i < ctx->layer_n; ++i)
    printf("\n\t%s", ctx->layers[i]);
  printf("\ninstance extensions: #%u", ctx->inst_ext_n);
  for (unsigned i = 0; i < ctx->inst_ext_n; ++i)
    printf("\n\t%s", ctx->inst_exts[i]);
  printf("\ndevice extensions: #%u", ctx->dev_ext_n);
  for (unsigned i = 0; i < ctx->dev_ext_n; ++i)
    printf("\n\t%s", ctx->dev_exts[i]);

  printf("\n--\n");
}

#endif /* defined(YF_DEBUG) */
