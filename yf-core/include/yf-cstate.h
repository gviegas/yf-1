/*
 * YF
 * yf-cstate.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_CSTATE_H
#define YF_YF_CSTATE_H

#include "yf/com/yf-defs.h"

#include "yf-stage.h"
#include "yf-dtable.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a compute state.
 */
typedef struct YF_cstate_o *YF_cstate;

/**
 * Type defining a compute state configuration.
 */
typedef struct {
  YF_stage stg;
  YF_dtable *dtbs;
  unsigned dtb_n;
} YF_cconf;

/**
 * Initializes a new compute state.
 *
 * @param ctx: The context.
 * @param conf: The configuration to use.
 * @return: On success, returns a new state. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_cstate yf_cstate_init(YF_context ctx, const YF_cconf *conf);

/**
 * Gets a compute state's stage.
 *
 * @param cst: The state.
 * @return: The stage.
 */
const YF_stage *yf_cstate_getstg(YF_cstate cst);

/**
 * Gets a compute state's dtable.
 *
 * @param cst: The state.
 * @param index: The index of the table to retrieve.
 * @return: The dtable.
 */
YF_dtable yf_cstate_getdtb(YF_cstate cst, unsigned index);

/**
 * Deinitializes a compute state.
 *
 * @param cst: The state to deinitialize. Can be 'NULL'.
 */
void yf_cstate_deinit(YF_cstate cst);

YF_DECLS_END

#endif /* YF_YF_CSTATE_H */
