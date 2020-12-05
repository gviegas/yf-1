/*
 * YF
 * yf-cmdbuf.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_CMDBUF_H
#define YF_YF_CMDBUF_H

#include "yf-common.h"
#include "yf-gstate.h"
#include "yf-cstate.h"
#include "yf-buffer.h"
#include "yf-image.h"

YF_DECLS_BEGIN

/* Opaque type defining a command buffer. */
typedef struct YF_cmdbuf_o *YF_cmdbuf;

/* Command buffer types. */
#define YF_CMDB_GRAPH 0
#define YF_CMDB_COMP  1

/* Begins a command buffer of a given type. */
YF_cmdbuf yf_cmdbuf_begin(YF_context ctx, int cmdb);

/* Ends a command buffer and enqueues it for execution. */
int yf_cmdbuf_end(YF_cmdbuf cmdb);

/* Executes pending command buffers. */
int yf_cmdbuf_exec(YF_context ctx);

/* Resets pending command buffers. */
void yf_cmdbuf_reset(YF_context ctx);

/* -- State -- */

/* Sets the graphics state. */
void yf_cmdbuf_setgstate(YF_cmdbuf cmdb, YF_gstate gst);

/* Sets the compute state. */
void yf_cmdbuf_setcstate(YF_cmdbuf cmdb, YF_cstate cst);

/* Sets the render target. */
void yf_cmdbuf_settarget(YF_cmdbuf cmdb, YF_target tgt);

/* Sets the viewport identified by 'index'. */
void yf_cmdbuf_setvport(
  YF_cmdbuf cmdb,
  unsigned index,
  const YF_viewport *vport);

/* Sets the scissor of the viewport identified by 'index'. */
void yf_cmdbuf_setsciss(YF_cmdbuf cmdb, unsigned index, YF_rect rect);

/* Sets a dtable resource allocation. */
void yf_cmdbuf_setdtable(YF_cmdbuf cmdb, unsigned index, unsigned alloc_i);

/* Sets a vertex buffer binding. */
void yf_cmdbuf_setvbuf(
  YF_cmdbuf cmdb,
  unsigned index,
  YF_buffer buf,
  size_t offset);

/* Sets the index buffer. */
void yf_cmdbuf_setibuf(
  YF_cmdbuf cmdb,
  YF_buffer buf,
  size_t offset,
  short stride);

/* -- Clear -- */

/* Clears the color aspect of the target's color image identified by 'index'. */
void yf_cmdbuf_clearcolor(YF_cmdbuf cmdb, unsigned index, YF_color value);

/* Clears the depth aspect of the target's depth/stencil image. */
void yf_cmdbuf_cleardepth(YF_cmdbuf cmdb, float value);

/* Clears the stencil aspect of the target's depth/stencil image. */
void yf_cmdbuf_clearsten(YF_cmdbuf cmdb, unsigned value);

/* -- Draw -- */

/* Draws primitives as specified by the bound state. */
void yf_cmdbuf_draw(
  YF_cmdbuf cmdb,
  int indexed,
  unsigned index_base,
  unsigned vert_n,
  unsigned inst_n,
  int vert_id,
  int inst_id);

/* -- Dispatch -- */

/* Dispatches a global workgroup with the given dimensions. */
void yf_cmdbuf_dispatch(YF_cmdbuf cmdb, YF_dim3 dim);

/* -- Copy -- */

/* Copies data between buffers. */
void yf_cmdbuf_copybuf(
  YF_cmdbuf cmdb,
  YF_buffer dst,
  size_t dst_offs,
  YF_buffer src,
  size_t src_offs,
  size_t size);

/* Copies data between images. */
void yf_cmdbuf_copyimg(
  YF_cmdbuf cmdb,
  YF_image dst,
  unsigned dst_layer,
  YF_image src,
  unsigned src_layer,
  unsigned layer_n);

/* -- Sync -- */

/* Synchronizes command buffer execution. */
void yf_cmdbuf_sync(YF_cmdbuf cmdb);

YF_DECLS_END

#endif /* YF_YF_CMDBUF_H */
