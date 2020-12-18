/*
 * YF
 * yf-gstate.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_GSTATE_H
#define YF_YF_GSTATE_H

#include <yf/com/yf-defs.h>

#include "yf-pass.h"
#include "yf-stage.h"
#include "yf-dtable.h"
#include "yf-vinput.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a graphics state.
 */
typedef struct YF_gstate_o *YF_gstate;

/**
 * Primitive type values.
 */
#define YF_PRIMITIVE_POINT    0
#define YF_PRIMITIVE_LINE     1
#define YF_PRIMITIVE_TRIANGLE 2

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
 * Front face values.
 */
#define YF_FRONTFACE_CW  0
#define YF_FRONTFACE_CCW 1

/**
 * Type defining a graphics state configuration.
 */
typedef struct {
  YF_pass pass;
  const YF_stage *stgs;
  unsigned stg_n;
  const YF_dtable *dtbs;
  unsigned dtb_n;
  const YF_vinput *vins;
  unsigned vin_n;
  int primitive;
  int polymode;
  int cullmode;
  int frontface;
} YF_gconf;

/**
 * Initializes a new graphics state.
 *
 * @param ctx: The context.
 * @param conf: The configuration to use.
 * @return: On success, returns a new state. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_gstate yf_gstate_init(YF_context ctx, const YF_gconf *conf);

/**
 * Gets the graphics state's dtable for the given index.
 *
 * @param gst: The state.
 * @param index: The index of the table to retrieve.
 * @return: The dtable.
 */
YF_dtable yf_gstate_getdtb(YF_gstate gst, unsigned index);

/**
 * Deinitializes a graphics state.
 *
 * @param gst: The state to deinitialize. Can be 'NULL'.
 */
void yf_gstate_deinit(YF_gstate gst);

YF_DECLS_END

#endif /* YF_YF_GSTATE_H */
