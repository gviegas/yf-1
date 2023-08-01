/*
 * YF
 * yf-cstate.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_CSTATE_H
#define YF_YF_CSTATE_H

#include "yf/com/yf-defs.h"

#include "yf-stage.h"
#include "yf-dtable.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a compute pipeline state.
 */
typedef struct yf_cstate yf_cstate_t;

/**
 * Type defining a compute state configuration.
 */
typedef struct yf_cconf {
    yf_stage_t stg;
    yf_dtable_t **dtbs;
    unsigned dtb_n;
} yf_cconf_t;

/**
 * Initializes a new compute state.
 *
 * @param ctx: The context.
 * @param conf: The configuration to use.
 * @return: On success, returns a new state. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_cstate_t *yf_cstate_init(yf_context_t *ctx, const yf_cconf_t *conf);

/**
 * Gets a compute state's stage.
 *
 * @param cst: The state.
 * @return: The stage.
 */
const yf_stage_t *yf_cstate_getstg(yf_cstate_t *cst);

/**
 * Gets a compute state's dtable.
 *
 * @param cst: The state.
 * @param index: The index of the table to retrieve.
 * @return: The dtable.
 */
yf_dtable_t *yf_cstate_getdtb(yf_cstate_t *cst, unsigned index);

/**
 * Deinitializes a compute state.
 *
 * @param cst: The state to deinitialize. Can be 'NULL'.
 */
void yf_cstate_deinit(yf_cstate_t *cst);

YF_DECLS_END

#endif /* YF_YF_CSTATE_H */
