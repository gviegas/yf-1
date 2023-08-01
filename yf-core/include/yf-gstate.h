/*
 * YF
 * yf-gstate.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_GSTATE_H
#define YF_YF_GSTATE_H

#include "yf/com/yf-defs.h"

#include "yf-pass.h"
#include "yf-stage.h"
#include "yf-dtable.h"
#include "yf-vinput.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a graphics pipeline state.
 */
typedef struct yf_gstate yf_gstate_t;

/**
 * Primitive topology values.
 */
#define YF_TOPOLOGY_POINT    0
#define YF_TOPOLOGY_LINE     1
#define YF_TOPOLOGY_TRIANGLE 2
#define YF_TOPOLOGY_LNSTRIP  3
#define YF_TOPOLOGY_TRISTRIP 4
#define YF_TOPOLOGY_TRIFAN   5

/**
 * Polygon mode values.
 */
#define YF_POLYMODE_FILL  0
#define YF_POLYMODE_LINE  1
#define YF_POLYMODE_POINT 2

/**
 * Cull mode values.
 */
#define YF_CULLMODE_NONE  0
#define YF_CULLMODE_FRONT 1
#define YF_CULLMODE_BACK  2
#define YF_CULLMODE_ANY   3

/**
 * Winding values.
 */
#define YF_WINDING_CW  0
#define YF_WINDING_CCW 1

/**
 * Type defining a graphics state configuration.
 */
typedef struct yf_gconf {
    yf_pass_t *pass;
    const yf_stage_t *stgs;
    unsigned stg_n;
    yf_dtable_t *const *dtbs;
    unsigned dtb_n;
    const yf_vinput_t *vins;
    unsigned vin_n;
    int topology;
    int polymode;
    int cullmode;
    int winding;
} yf_gconf_t;

/**
 * Initializes a new graphics state.
 *
 * @param ctx: The context.
 * @param conf: The configuration to use.
 * @return: On success, returns a new state. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_gstate_t *yf_gstate_init(yf_context_t *ctx, const yf_gconf_t *conf);

/**
 * Gets a graphics state's pass.
 *
 * @param gst: The state.
 * @return: The pass.
 */
yf_pass_t *yf_gstate_getpass(yf_gstate_t *gst);

/**
 * Gets a graphics state's stage.
 *
 * @param gst: The state.
 * @param stage: The 'YF_STAGE' value indicating the stage type to retrieve.
 * @return: The stage, or 'NULL' if 'gst' does not have a shader stage of
 *  the given type.
 */
const yf_stage_t *yf_gstate_getstg(yf_gstate_t *gst, int stage);

/**
 * Gets a graphics state's dtable.
 *
 * @param gst: The state.
 * @param index: The index of the table to retrieve.
 * @return: The dtable.
 */
yf_dtable_t *yf_gstate_getdtb(yf_gstate_t *gst, unsigned index);

/**
 * Deinitializes a graphics state.
 *
 * @param gst: The state to deinitialize. Can be 'NULL'.
 */
void yf_gstate_deinit(yf_gstate_t *gst);

YF_DECLS_END

#endif /* YF_YF_GSTATE_H */
