/*
 * YF
 * yf-pass.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_PASS_H
#define YF_YF_PASS_H

#include <yf/com/yf-defs.h>

#include "yf-context.h"
#include "yf-image.h"

YF_DECLS_BEGIN

/* Opaque type defining a render pass configuration. */
typedef struct YF_pass_o *YF_pass;

/* Opaque type defining a render target for a pass. */
typedef struct YF_target_o *YF_target;

/* Load operations. */
#define YF_LOADOP_UNDEF 0
#define YF_LOADOP_LOAD  1

/* Store operations. */
#define YF_STOREOP_UNDEF 0
#define YF_STOREOP_STORE 1

/* Type describing a color attachment for use in a pass. */
typedef struct {
  int pixfmt;
  int samples;
  int loadop;
  int storeop;
} YF_colordsc;

/* Type describing a depth/stencil attachment for use in a pass. */
typedef struct {
  int pixfmt;
  int samples;
  int depth_loadop;
  int depth_storeop;
  int stencil_loadop;
  int stencil_storeop;
} YF_depthdsc;

/* Type defining the resource of a target's attachment. */
typedef struct {
  YF_image img;
  unsigned layer_base;
} YF_attach;

/* Initializes a new pass. */
YF_pass yf_pass_init(
  YF_context ctx,
  const YF_colordsc *colors,
  unsigned color_n,
  const YF_colordsc *resolves,
  const YF_depthdsc *depth_stencil);

/* Makes a new target for use with a given pass. */
YF_target yf_pass_maketarget(
  YF_pass pass,
  YF_dim2 dim,
  unsigned layers,
  const YF_attach *colors,
  unsigned color_n,
  const YF_attach *resolves,
  const YF_attach *depth_stencil);

/* Unmakes a pass' target. */
int yf_pass_unmktarget(YF_pass pass, YF_target tgt);

/* Deinitializes a pass. */
void yf_pass_deinit(YF_pass pass);

YF_DECLS_END

#endif /* YF_YF_PASS_H */
