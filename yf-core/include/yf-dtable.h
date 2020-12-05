/*
 * YF
 * yf-dtable.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_DTABLE_H
#define YF_YF_DTABLE_H

#include "yf-common.h"
#include "yf-context.h"
#include "yf-buffer.h"
#include "yf-image.h"

YF_DECLS_BEGIN

/* Opaque type defining a descritor table. */
typedef struct YF_dtable_o *YF_dtable;

/* Descriptor types. */
#define YF_DTYPE_UNIFORM  0
#define YF_DTYPE_MUTABLE  1
#define YF_DTYPE_IMAGE    2
#define YF_DTYPE_SAMPLED  3
#define YF_DTYPE_SAMPLER  4
#define YF_DTYPE_ISAMPLER 5

/* Type defining an entry in a descriptor table. */
typedef struct {
  unsigned binding;
  int dtype;
  unsigned elements;
  void *info;
} YF_dentry;

/* Initializes a new descriptor table. */
YF_dtable yf_dtable_init(
  YF_context ctx,
  const YF_dentry *entries,
  unsigned entry_n);

/* Allocates resources for a given descriptor table. */
int yf_dtable_alloc(YF_dtable dtb, unsigned n);

/* Deallocates resources of a given descriptor table. */
void yf_dtable_dealloc(YF_dtable dtb);

/* Copies buffer data to a descriptor. */
int yf_dtable_copybuf(
  YF_dtable dtb,
  unsigned alloc_i,
  unsigned binding,
  YF_slice elements,
  const YF_buffer *bufs,
  const size_t *offsets,
  const size_t *sizes);

/* Copies image data to a descriptor. */
int yf_dtable_copyimg(
  YF_dtable dtb,
  unsigned alloc_i,
  unsigned binding,
  YF_slice elements,
  const YF_image *imgs,
  const unsigned *layers);

/* Deinitializes a descriptor table. */
void yf_dtable_deinit(YF_dtable dtb);

YF_DECLS_END

#endif /* YF_YF_DTABLE_H */
