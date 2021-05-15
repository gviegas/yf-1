/*
 * YF
 * yf-cmdbuf.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_CMDBUF_H
#define YF_YF_CMDBUF_H

#include "yf/com/yf-defs.h"

#include "yf-gstate.h"
#include "yf-cstate.h"
#include "yf-buffer.h"
#include "yf-image.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a command buffer.
 *
 * Command buffers are context-managed objects into which graphics or
 * compute commands can be encoded.
 * Work is performed through explicit execution of the context's pending
 * command buffers.
 */
typedef struct YF_cmdbuf_o *YF_cmdbuf;

/**
 * Command buffer types.
 */
#define YF_CMDBUF_GRAPH 0
#define YF_CMDBUF_COMP  1

/**
 * Gets a command buffer of a given type.
 *
 * The returned command buffer must only receive encodings valid for its
 * type, otherwise 'yf_cmdbuf_end()' will fail.
 *
 * @param ctx: The context.
 * @param cmdb: The 'YF_CMDBUF' value indicating the command buffer type.
 * @return: On success, returns a command buffer ready for encoding. Otherwise,
 *  'NULL' is returned and the global error is set to indicate the cause.
 */
YF_cmdbuf yf_cmdbuf_get(YF_context ctx, int cmdbuf);

/**
 * Ends a command buffer and enqueues it for execution.
 *
 * After a call to this function, the ended command buffer must not be used
 * any further, no matter the outcome.
 *
 * @param cmdb: The command buffer to end.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_cmdbuf_end(YF_cmdbuf cmdb);

/**
 * Executes pending command buffers.
 *
 * Resources are once again available for use after this function completes.
 *
 * @param ctx: The context that owns the command buffers to execute.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_cmdbuf_exec(YF_context ctx);

/**
 * Resets pending command buffers.
 *
 * Resources are once again available for use after this function completes.
 *
 * @param ctx: The context that owns the command buffers to reset.
 */
void yf_cmdbuf_reset(YF_context ctx);

/*
 * State
 */

/**
 * Sets the graphics state.
 *
 * @param cmdb: The command buffer.
 * @param gst: The state to set.
 */
void yf_cmdbuf_setgstate(YF_cmdbuf cmdb, YF_gstate gst);

/**
 * Sets the compute state.
 *
 * @param cmdb: The command buffer.
 * @param cst: The state to set.
 */
void yf_cmdbuf_setcstate(YF_cmdbuf cmdb, YF_cstate cst);

/**
 * Sets the render target.
 *
 * @param cmdb: The command buffer.
 * @param tgt: The target to set.
 */
void yf_cmdbuf_settarget(YF_cmdbuf cmdb, YF_target tgt);

/**
 * Sets a viewport.
 *
 * @param cmdb: The command buffer.
 * @param index: The viewport index.
 * @param vport: The viewport.
 */
void yf_cmdbuf_setvport(YF_cmdbuf cmdb, unsigned index,
                        const YF_viewport *vport);

/**
 * Sets the scissor of a viewport.
 *
 * @param cmdb: The command buffer.
 * @param index: The viewport index.
 * @param rect: The scissor rect.
 */
void yf_cmdbuf_setsciss(YF_cmdbuf cmdb, unsigned index, YF_rect rect);

/**
 * Sets a dtable resource allocation.
 *
 * @param cmdb: The command buffer.
 * @param index: The index of the table to set within the current state.
 * @param alloc_i: The index of the allocation within the table.
 */
void yf_cmdbuf_setdtable(YF_cmdbuf cmdb, unsigned index, unsigned alloc_i);

/**
 * Sets a vertex buffer binding.
 *
 * @param cmdb: The command buffer.
 * @param index: The index of the input binding.
 * @param buf: The vertex buffer.
 * @param offset: The offset from the beginning of the buffer.
 */
void yf_cmdbuf_setvbuf(YF_cmdbuf cmdb, unsigned index, YF_buffer buf,
                       size_t offset);

/**
 * Sets the index buffer.
 *
 * @param cmdb: The command buffer.
 * @param buf: The index buffer.
 * @param offset: The offset from the beginning of the buffer.
 * @param stride: The stride between adjacent indices (i.e., the size of the
 *  underlying type).
 */
void yf_cmdbuf_setibuf(YF_cmdbuf cmdb, YF_buffer buf, size_t offset,
                       unsigned stride);

/*
 * Clear
 */

/**
 * Clears the color aspect of the target's color image.
 *
 * @param cmdb: The command buffer.
 * @param index: The index of the attachment to clear.
 * @param value: The clear value.
 */
void yf_cmdbuf_clearcolor(YF_cmdbuf cmdb, unsigned index, YF_color value);

/**
 * Clears the depth aspect of the target's depth/stencil image.
 *
 * @param cmdb: The command buffer.
 * @param value: The clear value.
 */
void yf_cmdbuf_cleardepth(YF_cmdbuf cmdb, float value);

/**
 * Clears the stencil aspect of the target's depth/stencil image.
 *
 * @param cmdb: The command buffer.
 * @param value: The clear value.
 */
void yf_cmdbuf_clearsten(YF_cmdbuf cmdb, unsigned value);

/*
 * Drawing
 */

/**
 * Draws primitives as specified by the bound state.
 *
 * @param cmdb: The command buffer.
 * @param indexed: Whether or not this is an indexed draw.
 * @param index_base: The base index.
 * @param vert_n: The vertex count.
 * @param inst_n: The instance count.
 * @param vert_id: The starting vertex ID for use in the shader.
 * @param inst_id: The starting instance ID for use in the shader.
 */
void yf_cmdbuf_draw(YF_cmdbuf cmdb, int indexed, unsigned index_base,
                    unsigned vert_n, unsigned inst_n, int vert_id, int inst_id);

/*
 * Dispatching
 */

/**
 * Dispatches a global workgroup with the given dimensions.
 *
 * @param cmdb: The command buffer.
 * @param dim: The dimensions of the work group.
 */
void yf_cmdbuf_dispatch(YF_cmdbuf cmdb, YF_dim3 dim);

/*
 * Copy
 */

/**
 * Copies data between buffers.
 *
 * @param cmdb: The command buffer.
 * @param dst: The destination buffer.
 * @param dst_offs: The offset from the beginning of the destination buffer.
 * @param src: The source buffer.
 * @param src_offs: The offset from the beginning of the source buffer.
 * @param size: The number of bytes to copy.
 */
void yf_cmdbuf_copybuf(YF_cmdbuf cmdb, YF_buffer dst, size_t dst_offs,
                       YF_buffer src, size_t src_offs, size_t size);

/**
 * Copies data between images.
 *
 * @param cmdb: The command buffer.
 * @param dst: The destination image.
 * @param dst_off: The offset of the destination image.
 * @param dst_layer: The first layer of the destination image.
 * @param dst_level: The mip level of the destination image.
 * @param src: The source image.
 * @param src_off: The offset of the source image.
 * @param src_layer: The first layer of the source image.
 * @param src_level: The mip level of the source image.
 * @param dim: The extent to copy.
 * @param layer_n: The number of layers to copy.
 */
void yf_cmdbuf_copyimg(YF_cmdbuf cmdb, YF_image dst, YF_off3 dst_off,
                       unsigned dst_layer, unsigned dst_level,
                       YF_image src, YF_off3 src_off, unsigned src_layer,
                       unsigned src_level, YF_dim3 dim, unsigned layer_n);

/*
 * Synchronization
 */

/**
 * Synchronizes command buffer execution.
 *
 * @param cmdb: The command buffer.
 */
void yf_cmdbuf_sync(YF_cmdbuf cmdb);

YF_DECLS_END

#endif /* YF_YF_CMDBUF_H */
