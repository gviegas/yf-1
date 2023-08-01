/*
 * YF
 * yf-cmdbuf.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
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
 * Command buffers are context-managed objects that encode a set of
 * operations for future execution.
 */
typedef struct yf_cmdbuf yf_cmdbuf_t;

/**
 * Command buffer types.
 */
#define YF_CMDBUF_GRAPH 0
#define YF_CMDBUF_COMP  1
#define YF_CMDBUF_XFER  2

/**
 * Gets a command buffer of a given type.
 *
 * The returned command buffer must only receive encodings valid for its
 * type, otherwise 'yf_cmdbuf_end()' will fail.
 *
 * @param ctx: The context.
 * @param cmdbuf: The 'YF_CMDBUF' value indicating the command buffer type.
 * @return: On success, returns a command buffer ready for encoding. Otherwise,
 *  'NULL' is returned and the global error is set to indicate the cause.
 */
yf_cmdbuf_t *yf_cmdbuf_get(yf_context_t *ctx, int cmdbuf);

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
int yf_cmdbuf_end(yf_cmdbuf_t *cmdb);

/**
 * Executes pending command buffers.
 *
 * Resources are once again available for use after this function completes.
 *
 * @param ctx: The context that owns the command buffers to execute.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_cmdbuf_exec(yf_context_t *ctx);

/**
 * Resets pending command buffers.
 *
 * Resources are once again available for use after this function completes.
 *
 * @param ctx: The context that owns the command buffers to reset.
 */
void yf_cmdbuf_reset(yf_context_t *ctx);

/*
 * State
 */

/**
 * Sets the graphics state.
 *
 * CMDBUF_GRAPH
 *
 * @param cmdb: The command buffer.
 * @param gst: The state to set.
 */
void yf_cmdbuf_setgstate(yf_cmdbuf_t *cmdb, yf_gstate_t *gst);

/**
 * Sets the compute state.
 *
 * CMDBUF_COMP
 *
 * @param cmdb: The command buffer.
 * @param cst: The state to set.
 */
void yf_cmdbuf_setcstate(yf_cmdbuf_t *cmdb, yf_cstate_t *cst);

/**
 * Sets the render target.
 *
 * CMDBUF_GRAPH
 *
 * @param cmdb: The command buffer.
 * @param tgt: The target to set.
 */
void yf_cmdbuf_settarget(yf_cmdbuf_t *cmdb, yf_target_t *tgt);

/**
 * Sets a viewport.
 *
 * CMDBUF_GRAPH
 *
 * @param cmdb: The command buffer.
 * @param index: The viewport index.
 * @param vport: The viewport.
 */
void yf_cmdbuf_setvport(yf_cmdbuf_t *cmdb, unsigned index,
                        const yf_viewport_t *vport);

/**
 * Sets the scissor of a viewport.
 *
 * CMDBUF_GRAPH
 *
 * @param cmdb: The command buffer.
 * @param index: The viewport index.
 * @param rect: The scissor rect.
 */
void yf_cmdbuf_setsciss(yf_cmdbuf_t *cmdb, unsigned index, yf_rect_t rect);

/**
 * Sets a dtable resource allocation.
 *
 * CMDBUF_GRAPH
 * CMDBUF_COMP
 *
 * @param cmdb: The command buffer.
 * @param index: The index of the table to set within the current state.
 * @param alloc_i: The index of the allocation within the table.
 */
void yf_cmdbuf_setdtable(yf_cmdbuf_t *cmdb, unsigned index, unsigned alloc_i);

/**
 * Sets a vertex buffer binding.
 *
 * CMDBUF_GRAPH
 *
 * @param cmdb: The command buffer.
 * @param index: The index of the input binding.
 * @param buf: The vertex buffer.
 * @param offset: The offset from the beginning of the buffer.
 */
void yf_cmdbuf_setvbuf(yf_cmdbuf_t *cmdb, unsigned index, yf_buffer_t *buf,
                       size_t offset);

/**
 * Index types.
 */
#define YF_ITYPE_USHORT 0
#define YF_ITYPE_UINT   1

/**
 * Sets the index buffer.
 *
 * CMDBUF_GRAPH
 *
 * @param cmdb: The command buffer.
 * @param buf: The index buffer.
 * @param offset: The offset from the beginning of the buffer.
 * @param itype: The 'YF_ITYPE' value indicating the index type.
 */
void yf_cmdbuf_setibuf(yf_cmdbuf_t *cmdb, yf_buffer_t *buf, size_t offset,
                       int itype);

/*
 * Clear
 */

/**
 * Clears the color aspect of the target's color image.
 *
 * CMDBUF_GRAPH
 *
 * @param cmdb: The command buffer.
 * @param index: The index of the attachment to clear.
 * @param value: The clear value.
 */
void yf_cmdbuf_clearcolor(yf_cmdbuf_t *cmdb, unsigned index, yf_color_t value);

/**
 * Clears the depth aspect of the target's depth/stencil image.
 *
 * CMDBUF_GRAPH
 *
 * @param cmdb: The command buffer.
 * @param value: The clear value.
 */
void yf_cmdbuf_cleardepth(yf_cmdbuf_t *cmdb, float value);

/**
 * Clears the stencil aspect of the target's depth/stencil image.
 *
 * CMDBUF_GRAPH
 *
 * @param cmdb: The command buffer.
 * @param value: The clear value.
 */
void yf_cmdbuf_clearsten(yf_cmdbuf_t *cmdb, unsigned value);

/*
 * Drawing
 */

/**
 * Draws primitives.
 *
 * CMDBUF_GRAPH
 *
 * @param cmdb: The command buffer.
 * @param vert_id: The initial vertex ID for use in the shader.
 * @param vert_n: The number of vertices to draw.
 * @param inst_id: The initial instance ID for use in the shader.
 * @param inst_n: The number of instances to draw.
 */
void yf_cmdbuf_draw(yf_cmdbuf_t *cmdb, unsigned vert_id, unsigned vert_n,
                    unsigned inst_id, unsigned inst_n);

/**
 * Draws primitives using indices.
 *
 * CMDBUF_GRAPH
 *
 * @param cmdb: The command buffer.
 * @param index_base: The base index for index buffer access.
 * @param vert_off: The offset to use when computing the vertex ID.
 * @param vert_n: The number of vertices to draw.
 * @param inst_id: The initial instance ID for use in the shader.
 * @param inst_n: The number of instances to draw.
 */
void yf_cmdbuf_drawi(yf_cmdbuf_t *cmdb, unsigned index_base, int vert_off,
                     unsigned vert_n, unsigned inst_id, unsigned inst_n);

/*
 * Dispatching
 */

/**
 * Dispatches a global workgroup with the given dimensions.
 *
 * CMDBUF_COMP
 *
 * @param cmdb: The command buffer.
 * @param dim: The dimensions of the work group.
 */
void yf_cmdbuf_dispatch(yf_cmdbuf_t *cmdb, yf_dim3_t dim);

/*
 * Copy
 */

/**
 * Copies data between buffers.
 *
 * CMDBUF_XFER
 *
 * @param cmdb: The command buffer.
 * @param dst: The destination buffer.
 * @param dst_off: The offset from the beginning of the destination buffer.
 * @param src: The source buffer.
 * @param src_off: The offset from the beginning of the source buffer.
 * @param size: The number of bytes to copy.
 */
void yf_cmdbuf_copybuf(yf_cmdbuf_t *cmdb, yf_buffer_t *dst, size_t dst_off,
                       yf_buffer_t *src, size_t src_off, size_t size);

/**
 * Copies data between images.
 *
 * CMDBUF_XFER
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
void yf_cmdbuf_copyimg(yf_cmdbuf_t *cmdb, yf_image_t *dst, yf_off3_t dst_off,
                       unsigned dst_layer, unsigned dst_level,
                       yf_image_t *src, yf_off3_t src_off, unsigned src_layer,
                       unsigned src_level, yf_dim3_t dim, unsigned layer_n);

/*
 * Synchronization
 */

/**
 * Synchronizes command buffer execution.
 *
 * CMDBUF_GRAPH
 * CMDBUF_COMP
 * CMDBUF_XFER
 *
 * @param cmdb: The command buffer.
 */
void yf_cmdbuf_sync(yf_cmdbuf_t *cmdb);

YF_DECLS_END

#endif /* YF_YF_CMDBUF_H */
