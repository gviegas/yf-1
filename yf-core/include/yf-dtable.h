/*
 * YF
 * yf-dtable.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_DTABLE_H
#define YF_YF_DTABLE_H

#include "yf/com/yf-defs.h"

#include "yf-context.h"
#include "yf-buffer.h"
#include "yf-image.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a descriptor table.
 *
 * The descriptor table defines a set of resource types that can be used by
 * related shader programs.
 */
typedef struct YF_dtable_o *YF_dtable;

/**
 * Descriptor types.
 */
#define YF_DTYPE_UNIFORM  0
#define YF_DTYPE_MUTABLE  1
#define YF_DTYPE_IMAGE    2
#define YF_DTYPE_SAMPLED  3
#define YF_DTYPE_SAMPLER  4
#define YF_DTYPE_ISAMPLER 5

/**
 * Type defining an entry in a descriptor table.
 */
typedef struct {
    unsigned binding;
    int dtype;
    unsigned elements;
    void *info;
} YF_dentry;

/**
 * Initializes a new descriptor table.
 *
 * @param ctx: The context.
 * @param entries: The table entries.
 * @param entry_n: The entry count.
 * @return: On success, returns a new dtable. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_dtable yf_dtable_init(YF_context ctx, const YF_dentry *entries,
                         unsigned entry_n);

/**
 * Allocates resources for a given descriptor table.
 *
 * Initialization only defines the layout of a dtable. The memory required
 * for descriptor resources is allocated using this function.
 *
 * During command encoding, one specifies the index of the allocation to use.
 * Since commands are encoded in advance, multiple copies of resources are
 * necessary for operations requiring different values, as the memory contents
 * are applied at execution time, rather than encoding time.
 *
 * Multiple calls to this function replaces, rather than increases, the number
 * of available allocations. Any previous contents written to a given
 * allocation should be considered invalid.
 *
 * @param dtb: The dtable.
 * @param n: The number of copies to allocate.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_dtable_alloc(YF_dtable dtb, unsigned n);

/**
 * Deallocates resources of a given descriptor table.
 *
 * @param dtb: The dtable.
 */
void yf_dtable_dealloc(YF_dtable dtb);

/**
 * Copies buffer data to a descriptor.
 *
 * @param dtb: The dtable.
 * @param alloc_i: The index of the destination allocation.
 * @param binding: The binding number identifying the descriptor to update.
 * @param elements: The range of elements to update.
 * @param bufs: The source buffers, one for each element.
 * @param offsets: The offsets for each buffer.
 * @param sizes: The number of bytes to copy for each buffer.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_dtable_copybuf(YF_dtable dtb, unsigned alloc_i, unsigned binding,
                      YF_slice elements, const YF_buffer *bufs,
                      const size_t *offsets, const size_t *sizes);

/**
 * Copies image data to a descriptor.
 *
 * @param dtb: The dtable.
 * @param alloc_i: The index of the destination allocation.
 * @param binding: The binding number identifying the descriptor to update.
 * @param elements: The range of elements to update.
 * @param bufs: The source images, one for each element.
 * @param layers: The source layers, one for each image.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_dtable_copyimg(YF_dtable dtb, unsigned alloc_i, unsigned binding,
                      YF_slice elements, const YF_image *imgs,
                      const unsigned *layers);

/**
 * Deinitializes a descriptor table.
 *
 * @param dtb: The dtable to deinitialize. Can be 'NULL'.
 */
void yf_dtable_deinit(YF_dtable dtb);

YF_DECLS_END

#endif /* YF_YF_DTABLE_H */
