/*
 * YF
 * yf-cstate.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_CSTATE_H
#define YF_YF_CSTATE_H

#include "yf-common.h"
#include "yf-stage.h"
#include "yf-dtable.h"

YF_DECLS_BEGIN

/* Opaque type defining a compute state. */
typedef struct YF_cstate_o *YF_cstate;

/* Type defining a compute state configuration. */
typedef struct {
  YF_stage stg;
  YF_dtable *dtbs;
  unsigned dtb_n;
} YF_cconf;

/* Initializes a new compute state. */
YF_cstate yf_cstate_init(YF_context ctx, const YF_cconf *conf);

/* Gets the compute state's dtable for the given 'index'. */
YF_dtable yf_cstate_getdtb(YF_cstate cst, unsigned index);

/* Deinitializes a compute state. */
void yf_cstate_deinit(YF_cstate cst);

YF_DECLS_END

#endif /* YF_YF_CSTATE_H */
